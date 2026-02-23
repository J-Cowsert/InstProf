#include "Profiler.h"

#include "core/Assert.h"
#include "core/Callsite.h"
#include "core/Event.h"
#include "core/Log.h"
#include "core/System.h"
#include "core/Callsite.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <fstream>


#ifndef IP_EXPORT_TRACE 
    #define IP_EXPORT_TRACE 1
#endif


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

    // For future iterations look into flat maps for better cache locality
    struct DataBlock {
  
        std::unordered_map<uint32_t, ThreadState> perThreadData; // <tid, ThreadState>
    };

    static std::unique_ptr<DataBlock> s_Data = nullptr;  

    Profiler::Profiler() 
        : m_MainThreadID(GetCurrentThreadID()), m_Epoch(GetTime())      
    { 

        IP_ASSERT(!s_Data, "");
        s_Data = std::make_unique<DataBlock>();

        StartWorkerThread();
    }

    Profiler::~Profiler() {

        EndWorkerThread();
    }

    void Profiler::EnqueueEvent(const EventItem& event) {

        m_EventQueue.Push(event);
    }

    AggregateStats Profiler::GetStats(const CallsiteInfo* cs) {

        std::lock_guard lock(m_StatsMutex);
        return cs->stats;
    }

    CallsiteInfo* Profiler::FindCallsite(const char* name) {

        for (auto** p = &__start_instprof_cs; p < &__stop_instprof_cs; ++p) {
            if (std::strcmp((*p)->name, name) == 0) return *p;
        }
        return nullptr;
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

    #if IP_EXPORT_TRACE

        IP_INFO("Begin Trace");

        std::ofstream File("iptrace.json", std::ios::binary | std::ios::trunc);
        bool first = true;
    #endif

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
                        activeZones.pop_back(); // remove zone

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

                        // Aggregate Stats — written directly into the callsite
                        {
                            auto* cs = reinterpret_cast<CallsiteInfo*>(rec.callsiteInfo);
                            std::lock_guard lock(m_StatsMutex);
                            cs->stats.totalInclusiveTime += rec.inclusiveTime;
                            cs->stats.totalSelfTime      += rec.selfTime;
                            cs->stats.maxInclusiveTime   = std::max(cs->stats.maxInclusiveTime, rec.inclusiveTime);
                            cs->stats.maxSelfTime        = std::max(cs->stats.maxSelfTime, rec.selfTime);
                            cs->stats.callCount++;
                        }

                        // Temporary Trace-Event-Format Export for project
                        #if IP_EXPORT_TRACE                                             
                        if (first) { File << "[\n"; first = false; }
                        else       { File << ",\n"; }

                        if (File.is_open()) {
                            const auto* cs = reinterpret_cast<const CallsiteInfo*>(rec.callsiteInfo);

                            File << "  {"
                                << "\"name\": \"" << (cs ? cs->name : "unknown") << "\","
                                << "\"cat\": \"function\","
                                << "\"ph\": \"X\","
                                << "\"ts\": " << rec.startTime / 1000 << ","
                                << "\"dur\": " << rec.inclusiveTime / 1000 << ","
                                << "\"pid\": 0,"
                                << "\"tid\": " << rec.threadID << ",";

                            File << "\"args\": { \"self\": " << rec.selfTime / 1000 << " }"
                                << "}";
                        }
                        #endif

                        break;
                    } 
                }
            } 
            else {

                if (m_Stop.load()) break; 
                //std::this_thread::yield();
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }

    #if IP_EXPORT_TRACE
        File << "\n]\n";
        File.flush();
    #endif
    }

    void Profiler::DebugLogDrainQueue() {

        EventItem ev;
        size_t count = 0;

        while (m_EventQueue.TryPop(ev)) {
            
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

            count++;
        }

        IP_INFO("DebugDrainQueue: Drained {} event(s).", count);
    }

    void Profiler::DebugLogDumpZones() {

        for (auto& [tid, tState] : s_Data->perThreadData) {
            
            const auto& zones = tState.completedZones;

            for (const auto& r : zones) {

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
        
    }

}
