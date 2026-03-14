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
#include <mutex>

/*
    Look into std::stacktrace
*/

namespace instprof {

    struct ThreadEntry {

        uint32_t ThreadID;
        SPSCQueue<EventItem, 4096> EventQueue;
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
            
            thread_local ThreadEntry* s = nullptr;
            if (!s) [[unlikely]] s = RegisterThread();

            while (!s->EventQueue.TryPush(event)) { m_PushFailCount.fetch_add(1, std::memory_order_relaxed); }
        }

    private:
        Profiler();
        ~Profiler();
        
        // noinline
        ThreadEntry* RegisterThread() {

            ThreadEntry* entry = new ThreadEntry{
                .ThreadID = GetCurrentThreadID(), 
                .EventQueue = {}
            };

            {
                std::scoped_lock lock(m_RegistrationMutex); 
                m_ThreadEntries.push_back(entry);
            }
            
            return entry;
        }

        bool StartWorkerThread();
        void EndWorkerThread();
        void ProcessEvents();

        void ExportTrace();
        void PrintStatsReport();
        
    private:
        uint32_t m_MainThreadID;
        int64_t m_Epoch;

        std::atomic<bool> m_Running{false}, m_Stop{false}; // m_running doesnt need to be atomic
        std::thread m_Worker;
     
        std::mutex m_RegistrationMutex;
        std::vector<ThreadEntry*> m_ThreadEntries;


        // Debug
        std::atomic<uint64_t> m_PushFailCount{0};
    };

    // Linker-generated section boundaries — array of CallsiteInfo pointers
    extern "C" {
        extern CallsiteInfo* __start_instprof_cs;
        extern CallsiteInfo* __stop_instprof_cs;
    }

}
