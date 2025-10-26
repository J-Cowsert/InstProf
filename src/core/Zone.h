#pragma once

#include "core/Profiler.h"
#include "core/Callsite.h"
#include "core/Event.h"
#include "core/System.h"

namespace instprof {

    class ZoneScope {
    public:
        
        explicit IP_FORCE_INLINE ZoneScope(const CallsiteInfo* info) {

            EventItem e{};
            e.tag.type = EventType::ZoneBegin;
            e.zoneBegin.time         = GetTime();
            e.zoneBegin.callsiteInfo = reinterpret_cast<uint64_t>(info);
            e.zoneBegin.threadID     = GetCurrentThreadID();
            Profiler::Get().EnqueueEvent(e);
        }

        IP_FORCE_INLINE ~ZoneScope() {
            
            EventItem e{};
            e.tag.type = EventType::ZoneEnd;
            e.zoneEnd.time     = GetTime();
            e.zoneEnd.threadID = GetCurrentThreadID();
            Profiler::Get().EnqueueEvent(e);
        }

        ZoneScope(const ZoneScope&) = delete;
        ZoneScope& operator=(const ZoneScope&) = delete;
        ZoneScope(ZoneScope&&) = delete;
        ZoneScope& operator=(ZoneScope&&) = delete;
    };
}
