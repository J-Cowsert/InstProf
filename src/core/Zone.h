#pragma once

#include "core/Profiler.h"
#include "core/Callsite.h"
#include "core/Event.h"
#include "core/System.h"

namespace instprof {

    // Notes: Consider single event design tradoffs. Sending two events makes live-view easy.
    //
    // Currently when a ZoneBegin event is sent to a queue that is full, EnqueueEvent spins until a slot is available. 
    // This distorts the measurements. A single event design would solve this issue, and simplify consumer code at the
    // sacrifice of live-view in the future.
        
    class ZoneScope {
    public:
        
        explicit IP_FORCE_INLINE ZoneScope(const CallsiteInfo* info) {

            EventItem e;
            e.tag.type = EventType::ZoneBegin;
            e.zoneBegin.time         = GetTime();
            e.zoneBegin.callsiteInfo = reinterpret_cast<uintptr_t>(info);
            // e.zoneBegin.threadID     = GetCurrentThreadID();
            Profiler::Get().EnqueueEvent(e);
        }

        IP_FORCE_INLINE ~ZoneScope() {
            
            EventItem e;
            e.tag.type = EventType::ZoneEnd;
            e.zoneEnd.time     = GetTime();
            // e.zoneEnd.threadID = GetCurrentThreadID();
            Profiler::Get().EnqueueEvent(e);
        }

        ZoneScope(const ZoneScope&) = delete;
        ZoneScope& operator=(const ZoneScope&) = delete;
        ZoneScope(ZoneScope&&) = delete;
        ZoneScope& operator=(ZoneScope&&) = delete;
    };
}
