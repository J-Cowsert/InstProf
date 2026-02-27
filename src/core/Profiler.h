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
        static Profiler& Get() {

            static Profiler instance;
            return instance;
        }

        Profiler(const Profiler&) = delete;
        Profiler& operator=(const Profiler&) = delete;
        Profiler(Profiler&&) = delete;
        Profiler& operator=(Profiler&&) = delete;

        void EnqueueEvent(const EventItem& event);

    private:
        Profiler();
        ~Profiler();

    private:
        bool StartWorkerThread();
        void EndWorkerThread();
        void ProcessEvents();
        void PrintStatsReport();

    private:
        uint32_t m_MainThreadID;
        int64_t m_Epoch;
        TSRingBuffer<EventItem> m_EventQueue{1024}; 

        std::atomic<bool> m_Running{false}, m_Stop{false};
        std::thread m_Worker;
        std::mutex m_StatsMutex;
    };

    // Linker-generated section boundaries — array of CallsiteInfo pointers
    extern "C" {
        extern CallsiteInfo* __start_instprof_cs;
        extern CallsiteInfo* __stop_instprof_cs;
    }

}
