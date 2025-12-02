#pragma once

#include "Core.h"
#include "Event.h"
#include "TSRingBuffer.h"

#include <cstdint>
#include <atomic>

/*
    Look into std::stacktrace
*/

namespace instprof {

    struct ZoneRecord {

        uintptr_t callsiteInfo;          // TODO: think about some sort of id hash if im to serialize in the future
        int64_t  startTime;
        int64_t  endTime;
        int64_t  inclusiveTime;         // total duration (end-start)
        int64_t  selfTime;              // duration excluding child zones
        uint32_t threadID;
        uint16_t depth;                 // nesting depth
    };

    struct ActiveZone {

        uintptr_t callsiteInfo; // ptr
        int64_t  startTime;
        int64_t  childInclusiveTime = 0; // total time of direct children
        uint16_t depth = 0;
    };

    // TODO: think about a better allocation strategy depending on data flow for future iterations (slab, arena)
    struct ThreadState {

        std::vector<ActiveZone> activeZoneStack{  };    // currently open zones
        std::vector<ZoneRecord> completedZones{  };     // finalized samples for this thread
        uint16_t currentDepth = 0;

        ThreadState() { activeZoneStack.reserve(128); completedZones.reserve(8192); }
    };

    // Per-Callsite stats
    struct AggregateStats {

        uint64_t callCount = 0;
        int64_t  totalInclusiveTime = 0; // total wall-time
        int64_t  totalSelfTime = 0;
        int64_t  maxInclusiveTime = 0;
        int64_t  maxSelfTime = 0;
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
        TSRingBuffer<EventItem> m_EventQueue{1024}; // memory used: 1024 * sizeof(EventItem)

        std::atomic<bool> m_Running{false}, m_Stop{false};
        std::thread m_Worker;
    };

}
