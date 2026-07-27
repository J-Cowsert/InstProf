#pragma once

#include "Core.h"
#include "Callsite.h"
#include "Event.h"
#include "SPSCQueue.h"
#include "System.h"

#include <cstdint>
#include <thread>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>

namespace instprof {

    struct ZoneRecord {

        uintptr_t callsiteInfo;
        int64_t  startTime;
        int64_t  endTime;
        int64_t  inclusiveTime;         // total duration (end-start)
        int64_t  selfTime;              // duration excluding child zones
        uint32_t threadID;
        uint16_t depth;                 // nesting depth
    };

    struct ActiveZone {

        uintptr_t callsiteInfo; 
        int64_t  startTime;
        int64_t  childInclusiveTime = 0; // total time of direct children
        uint16_t depth = 0;
    };

    struct ThreadState {

        std::vector<ZoneRecord> completedZones;  // finalized samples 
        std::vector<ActiveZone> activeZoneStack; // currently open zones
        uint16_t currentDepth = 0;
    };

    struct ThreadEntry {

        SPSCQueue<EventItem, 2048> EventQueue;
        ThreadState State; 
        uint32_t ThreadID;
    };

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

        IP_FORCE_INLINE void EnqueueEvent(const EventItem& event) {
            
            thread_local ThreadEntry* entry = nullptr;
            if (!entry) [[unlikely]] entry = RegisterThread();

            while (!entry->EventQueue.TryPush(event)) { m_PushFailCount.fetch_add(1, std::memory_order_relaxed); }
        }

    private:
        Profiler();
        ~Profiler();
        
        // noinline
        ThreadEntry* RegisterThread() {

            ThreadEntry* entry = new ThreadEntry{
                .EventQueue = {},
                .ThreadID = GetCurrentThreadID()
            };

            {
                std::scoped_lock lock(m_RegistrationMutex); 
                m_ThreadEntries.push_back(entry);
                m_RegisteredThreadCount.fetch_add(1, std::memory_order_relaxed); // Relaxed is fine here because of the lock 
            }
            
            return entry;
        }

        bool StartWorkerThread();
        void EndWorkerThread();
        void ProcessEvents();

        void ExportTrace();
        void PrintStatsReport();
        
    private:
        std::mutex m_RegistrationMutex;
        std::vector<ThreadEntry*> m_ThreadEntries;

        std::thread m_Worker;

        int64_t m_Epoch;
        uint32_t m_MainThreadID;

        std::atomic<int32_t> m_RegisteredThreadCount{0};
        std::atomic<bool> m_Running{false}, m_Stop{false}; // m_running doesnt need to be atomic

        // Debug
        std::atomic<int64_t> m_PushFailCount{0};

    };

    // Linker-generated section boundaries — array of CallsiteInfo pointers
    extern "C" {
        extern CallsiteInfo* __start_instprof_cs;
        extern CallsiteInfo* __stop_instprof_cs;
    }

}
