#pragma once

#include "Core.h"
#include "Event.h"
#include "Callsite.h"
#include "TSRingBuffer.h"
#include "TraceEventWorker.h"

#include <cstdint>
#include <atomic>
#include <unordered_map>

/*
    Look into std::stacktrace
*/

namespace instprof {

    struct ZoneRecord {

        uint32_t threadID;
        const CallsiteInfo* info;
        uint32_t depth;                 // nesting depth
        int64_t  startTime;
        int64_t  endTime;
        int64_t  inclusiveTime;         // total duration (end-start)
        int64_t  selfTime;              // duration excluding child zones
    };

    struct ActiveZone {

        const CallsiteInfo* info;
        int64_t  startTime;
        int64_t  childInclusiveTime = 0; // total time of direct children
        uint32_t depth = 0;
    };

    struct ThreadState {

        std::vector<ActiveZone> activeZones;    // currently open zones
        std::vector<ZoneRecord> completedZones; // finalized samples for this thread
        uint32_t currentDepth = 0;
    };

    // Per-Callsite stats
    struct AggregateStats {

        uint64_t callCount = 0;
        int64_t  totalInclusiveTime = 0;
        int64_t  totalSelfTime = 0;
        int64_t  maxInclusiveTime = 0;
        int64_t  maxSelfTime = 0;
    };

    struct ProfilerState {

        std::unordered_map<uint32_t, ThreadState> perThread;
        std::unordered_map<const CallsiteInfo*, AggregateStats> perCallsite;
        int64_t startTimestamp = -1;
    };

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
        void StartWorkerThread();
        void DebugLogDrainQueue();
        
    private:
        uint32_t m_MainThreadID;
        int64_t m_Epoch;
        TSRingBuffer<EventItem> m_EventQueue{1024}; // Memory used: 1024 * sizeof(EventItem)

        std::atomic<bool> m_Running{false}, m_Stop{false};
        std::thread m_Worker;

        TraceEventWorker m_TraceExportWorker{m_EventQueue, "trace.json"};
    };

}
