#include "Profiler.h"

#include "Assert.h"
#include "core/Event.h"
#include "core/System.h"
#include "core/Callsite.h"

namespace instprof {

    Profiler::Profiler() 
        : m_MainThreadID(GetCurrentThreadID()), m_Epoch(GetTime())      
    { 

        bool ready = m_TraceExportWorker.Start();
        IP_ASSERT(ready, "Export worker failed to start");
    }

    Profiler::~Profiler() {

        m_TraceExportWorker.Stop();
    }

    // Hotpath
    void Profiler::EnqueueEvent(const EventItem& event) {

        m_EventQueue.Push(event);
    }

    void Profiler::DebugLogDrainQueue() {

        EventItem ev;
        size_t count = 0;

        while (m_EventQueue.TryPop(ev)) {
            switch (ev.tag.type) {

                case EventType::ZoneBegin: {
                    const auto* cs = reinterpret_cast<const CallsiteInfo*>(ev.zoneBegin.callsiteInfo);
                    IP_INFO("[{}] ZoneBegin t={} thread={} name='{}' func='{}' file='{}' line={}",
                        count,
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
                    IP_INFO("[{}] ZoneEnd t={} thread={}",
                        count,
                        ev.zoneEnd.time,
                        ev.zoneEnd.threadID
                    );
                    break;
                }

                default: {
                    IP_WARN("[{}] Unknown event type ({})", count, static_cast<int>(ev.tag.type));
                    break;
                }
            }

            ++count;
        }

        IP_INFO("DebugDrainQueue: Drained {} event(s).", count);
    }

}
