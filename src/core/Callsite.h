#pragma once 

#include "core/Core.h"

#include <cstdint>
#include <source_location>

namespace instprof {

    struct CallsiteInfo {

        const char* name; 
        const char* function; 
        const char* file;
        uint32_t line;
    };

    IP_FORCE_INLINE constexpr CallsiteInfo MakeCallsite(const char* name, std::source_location loc = std::source_location::current()) noexcept {

        return {
            name,
            loc.function_name(),
            loc.file_name(),
            static_cast<uint32_t>(loc.line())
        };
    }
    
}