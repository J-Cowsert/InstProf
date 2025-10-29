#pragma once

#include "Core.h"
#include "Event.h"
#include "TSRingBuffer.h"
#include "TraceEventWorker.h"

#include <cstdint>
#include <atomic>


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

    private:
        bool StartWorkerThread();
        void EndWorkerThread();
        void ProcessEvents();

    private:
        void DebugLogDrainQueue();
        void DebugLogDumpZones();
        
    private:
        uint32_t m_MainThreadID;
        int64_t m_Epoch;
        TSRingBuffer<EventItem> m_EventQueue{1024}; // memory used: 1024 * sizeof(EventItem)

        std::atomic<bool> m_Running{false}, m_Stop{false};
        std::thread m_Worker;

        TraceEventWorker m_TraceExportWorker{m_EventQueue, "trace.json"}; // temp json exporter
    };

}
