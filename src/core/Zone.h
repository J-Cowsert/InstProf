#pragma once

#include "Profiler.h"

namespace instprof {

    class ZoneScope {
    public:
        
        explicit IP_FORCE_INLINE ZoneScope(CallsiteInfo info) {

            m_DebugStartTime = Profiler::GetTime(); 
            m_Info = info;



        }

        IP_FORCE_INLINE ~ZoneScope() {

            int64_t debugTime = Profiler::GetTime() - m_DebugStartTime;
            
            IP_INFO("Name: {}, Function: {}, File: {}, Line: {}  -  {} ns", m_Info.name, m_Info.function, m_Info.file, m_Info.line, debugTime);
        }

        ZoneScope(const ZoneScope&) = delete;
        ZoneScope& operator=(const ZoneScope&) = delete;
        ZoneScope(ZoneScope&&) = delete;
        ZoneScope& operator=(ZoneScope&&) = delete;

        // temporary: this class shouldnt own any data
        CallsiteInfo m_Info;
        int64_t m_DebugStartTime = 0; 
    };
}
