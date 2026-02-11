#pragma once

#include "Core.h"
#include "Callsite.h"
#include "Event.h"
#include "TSRingBuffer.h"

#include <cstdint>
#include <atomic>
#include <mutex>

/*
    Look into std::stacktrace
*/

namespace instprof {

    class Profiler {
    public:

        Profiler();
        ~Profiler();

        static Profiler& Get() {

            static Profiler instance;
            return instance;
        }

        void EnqueueEvent(const EventItem& event);

        // Thread-safe snapshot of a callsite's stats.
        AggregateStats GetStats(const CallsiteInfo* cs);

        // One-time name lookup. Cache the returned pointer for repeated access.
        CallsiteInfo* FindCallsite(const char* name);

        // Iterate all registered callsites (discovered via linker section).
        template<typename F>
        void ForEachStat(F&& fn);

    private:
        bool StartWorkerThread();
        void EndWorkerThread();
        void ProcessEvents();

    private:
        void DebugLogDrainQueue();
        void DebugLogDumpZones();
        void DebugLogDumpAggregates();

    private:
        uint32_t m_MainThreadID;
        int64_t m_Epoch;
        TSRingBuffer<EventItem> m_EventQueue{1024}; // TODO: Get it to overflow and see what happens

        std::atomic<bool> m_Running{false}, m_Stop{false};
        std::thread m_Worker;
        std::mutex m_StatsMutex;
    };

    // Linker-generated section boundaries — array of CallsiteInfo pointers
    extern "C" {
        extern CallsiteInfo* __start_instprof_cs;
        extern CallsiteInfo* __stop_instprof_cs;
    }

    template<typename F>
    void Profiler::ForEachStat(F&& fn) {

        std::lock_guard lock(m_StatsMutex);
        for (auto** p = &__start_instprof_cs; p < &__stop_instprof_cs; ++p) {
            fn(*p, (*p)->stats);
        }
    }

}
