#pragma once

#include "Core.h"
#include "Log.h"

#include <cstdint>
#include <source_location>
#include <string_view>

/*
    std::source_location
    std::stacktrace
*/

namespace instprof {

    struct CallsiteInfo {

        std::string_view name; // TODO: THIS COULD DANGLE IF NOT PASSED IN AS STR LITERAL
        std::string_view function; 
        std::string_view file;
        uint32_t line;
    };

    IP_FORCE_INLINE constexpr CallsiteInfo make_callsite(std::string_view name, std::source_location loc = std::source_location::current()) noexcept {

        return { name, std::string_view{loc.function_name()}, std::string_view{loc.file_name()}, static_cast<uint32_t>(loc.line()) };
    }


    class Profiler {
    public:

        Profiler();
        ~Profiler() = default;

        static Profiler& GetProfiler() {
            
            static Profiler instance;  
            return instance;
        }
        
    private:

        uint32_t m_MainThreadID;

    };

}
