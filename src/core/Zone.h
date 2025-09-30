#pragma once

#include "Profiler.h"

#include <string_view>
#include <source_location>

namespace instprof {

    // Temp: shouldnt live here
    struct CallsiteInfo {

        std::string_view name; 
        std::string_view function; 
        std::string_view file;
        uint32_t line;
    };

    IP_FORCE_INLINE constexpr CallsiteInfo make_callsite(std::string_view name, std::source_location loc = std::source_location::current()) noexcept {

        return { name, std::string_view{loc.function_name()}, std::string_view{loc.file_name()}, static_cast<uint32_t>(loc.line()) };
    }


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
