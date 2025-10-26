#include "Profiler.h"

#include "core/Event.h"
#include "core/System.h"
#include "core/Callsite.h"

namespace instprof {

    Profiler::Profiler() 
        : m_MainThreadID(GetCurrentThreadID())       
    { 
    }

    Profiler::~Profiler() {

        DebugDrainQueue();
    }

    // Hotpath
    void Profiler::EnqueueEvent(const EventItem& e) {

        m_EventQueue.Push(e);
    }

    void Profiler::DebugDrainQueue() {

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
