#include "Profiler.h"

#include "core/Assert.h"
#include "core/Callsite.h"
#include "core/Event.h"
#include "core/Log.h"
#include "core/System.h"
#include "core/Callsite.h"

#include <cstdint>
#include "unordered_map"

namespace instprof {

    struct ZoneRecord {

        uint64_t callsiteInfo;          // ptr (TODO: think about some sort of id hash if im to serialize in the future)
        int64_t  startTime;
        int64_t  endTime;
        int64_t  inclusiveTime;         // total duration (end-start)
        int64_t  selfTime;              // duration excluding child zones
        uint32_t threadID;
        uint16_t depth;                 // nesting depth
    };

    struct ActiveZone {

        uint64_t callsiteInfo; // ptr
        int64_t  startTime;
        int64_t  childInclusiveTime = 0; // total time of direct children
        uint16_t depth = 0;
    };

    // TODO: think about a better allocation strategy depending on data flow at runtime long term. 
    //       Look into std::pmr::monotonic_buffer_resource. Maybe a good time to write ur own allocator (slab, arena)
    struct ThreadState {

        std::vector<ActiveZone> activeZoneStack;    // currently open zones
        std::vector<ZoneRecord> completedZones;     // finalized samples for this thread
        uint16_t currentDepth = 0;

        // TODO: Constructor here that reserves vec memory so they aren't constantly reallocating
    };

    // Per-Callsite stats
    struct AggregateStats {

        uint64_t callCount = 0;
        int64_t  totalInclusiveTime = 0; // total wall-time
        int64_t  totalSelfTime = 0;
        int64_t  maxInclusiveTime = 0;
        int64_t  maxSelfTime = 0;
    };

    // Shit cache locality cuz of ptr indirection. Need somehting like a flat map
    struct DataBlock {

        std::unordered_map<uint32_t, ThreadState> perThreadData;      // <tid, ThreadState>
        std::unordered_map<uint64_t, AggregateStats> perCallsiteData; // <callsiteInfo ptr, AggregateStats>
    };


    


    namespace Debug {
    
        void LogEventItem(const EventItem& ev) {

            switch (ev.tag.type) {

                case EventType::ZoneBegin: {
                    const auto* cs = reinterpret_cast<const CallsiteInfo*>(ev.zoneBegin.callsiteInfo);
                    IP_INFO("[EventItem] ZoneBegin t={} thread={} name='{}' func='{}' file='{}' line={}",
                        ev.zoneBegin.time,
                        ev.zoneBegin.threadID,
                        cs ? cs->name : "<null>",
                        cs ? cs->function : "<null>",
                        cs ? cs->file : "<null>",
                        cs ? cs->line : 0
                    );
                    break;
                }

                case EventType::ZoneEnd: {
                    IP_INFO("[EventItem] ZoneEnd t={} thread={}",
                        ev.zoneEnd.time,
                        ev.zoneEnd.threadID
                    );
                    break;
                }

                default: {
                    IP_WARN("[EventItem] Unknown event type ({})", static_cast<int>(ev.tag.type));
                    break;
                }
            }
        }

        void LogZoneRecord(const ZoneRecord& r) {

            const auto* cs = reinterpret_cast<const CallsiteInfo*>(r.callsiteInfo);

            IP_INFO("[ZoneRecord] tid={} depth={} start={} end={} incl={:.3f} ms self={:.3f} ms name='{}' func='{}' file='{}' line={}",
                r.threadID,
                r.depth,
                r.startTime,
                r.endTime,
                static_cast<double>(r.inclusiveTime) / 1e6,
                static_cast<double>(r.selfTime) / 1e6,
                cs ? cs->name : "<null>",
                cs ? cs->function : "<null>",
                cs ? cs->file : "<null>",
                cs ? cs->line : 0
            );
        }
    }
    

    static DataBlock* s_Data = nullptr; // Idk if you should lazy allocate this on first prof event? Fine for now

    Profiler::Profiler() 
        : m_MainThreadID(GetCurrentThreadID()), m_Epoch(GetTime())      
    { 

        IP_ASSERT(!s_Data, "");
        s_Data = new DataBlock();

        StartWorkerThread();

        //bool ready = m_TraceExportWorker.Start();
        //IP_ASSERT(ready, "Export worker failed to start");
    }

    Profiler::~Profiler() {

        EndWorkerThread();


        {
            size_t totalZones = 0;
            for (auto& [tid, tState] : s_Data->perThreadData) {
                IP_WARN("Thread {} has {} completed zone(s)", tid, tState.completedZones.size());
                totalZones += tState.completedZones.size();
            }

            IP_WARN("Total Completed Zone Count: {}\n\n", totalZones);
            DebugLogDumpZones();
        }

        delete s_Data;

        //m_TraceExportWorker.Stop();
    }

    // Hot
    void Profiler::EnqueueEvent(const EventItem& event) {

        m_EventQueue.Push(event);
    }

    bool Profiler::StartWorkerThread() {

        if (m_Running.load()) return false;

        m_Stop.store(false);
        m_Running.store(true);
        m_Worker = std::thread([this]{ ProcessEvents(); });
        return true;
    }

    void Profiler::EndWorkerThread() {

        if (!m_Running.load()) return;
        m_Stop.store(true);
        if (m_Worker.joinable()) m_Worker.join();
        m_Running.store(false);
    }

    void Profiler::ProcessEvents() {

        auto& Data = s_Data;

        EventItem ev;

        for (;;) {

            if (m_EventQueue.TryPop(ev)) {

                switch (ev.tag.type) {

                    case EventType::ZoneBegin:
                    {
                        auto& tState = Data->perThreadData[ev.zoneBegin.threadID];
                        auto& activeZones = tState.activeZoneStack;

                        auto& az = activeZones.emplace_back();
                        az.startTime          = ev.zoneBegin.time;
                        az.callsiteInfo       = ev.zoneBegin.callsiteInfo;
                        az.childInclusiveTime = 0;
                        az.depth              = (uint32_t)activeZones.size() - 1; // 0-based depth

                        break;
                    }

                    case EventType::ZoneEnd:
                    {
                        auto& tState = Data->perThreadData[ev.zoneEnd.threadID];
                        auto& activeZones = tState.activeZoneStack;

                        auto az = activeZones.back(); // Copy top of active stack
                        activeZones.pop_back(); // remove

                        auto& rec = tState.completedZones.emplace_back();
                        rec.startTime     = az.startTime;
                        rec.endTime       = ev.zoneEnd.time;
                        rec.callsiteInfo  = az.callsiteInfo;
                        rec.threadID      = ev.zoneEnd.threadID;
                        rec.depth         = activeZones.size(); // depth after pop
                        rec.inclusiveTime = rec.endTime - rec.startTime;
                        rec.selfTime      = rec.inclusiveTime - az.childInclusiveTime;
                        
                        if (!activeZones.empty()) {

                            activeZones.back().childInclusiveTime += rec.inclusiveTime; // propagate child inclusive time to direct parent
                        }

                        

                        // TODO: AGGREGETE STATS HERE


                        break;
                    } 
                }
            } 
            else {

                if (m_Stop.load()) break; 
                std::this_thread::yield();
            }
        }
    }

    void Profiler::DebugLogDrainQueue() {

        EventItem ev;
        size_t count = 0;

        while (m_EventQueue.TryPop(ev)) {
            
            Debug::LogEventItem(ev);
            count++;
        }

        IP_INFO("DebugDrainQueue: Drained {} event(s).", count);
    }

    void Profiler::DebugLogDumpZones() {

        for (auto& [tid, tState] : s_Data->perThreadData) {
            
            const auto& zones = tState.completedZones;

            for (const auto& z : zones) {

                Debug::LogZoneRecord(z);
            }
        }
        
    }

}
