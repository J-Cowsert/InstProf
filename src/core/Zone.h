#pragma once

#include "Profiler.h"

namespace instprof {

    class ZoneScope {
    public:
        
        explicit IP_FORCE_INLINE ZoneScope(CallsiteInfo info) {

        
        }

        IP_FORCE_INLINE ~ZoneScope() {

            
        }

        ZoneScope(const ZoneScope&) = delete;
        ZoneScope& operator=(const ZoneScope&) = delete;
        ZoneScope(ZoneScope&&) = delete;
        ZoneScope& operator=(ZoneScope&&) = delete;
    };
}
