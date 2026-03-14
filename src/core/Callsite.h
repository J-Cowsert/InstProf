#pragma once

#include "core/Core.h"

#include <cstdint>
#include <source_location>

#define IP_REGISTER_CALLSITE(cs) static __attribute__((section("instprof_cs"), used)) instprof::CallsiteInfo* IP_CONCAT(_ipCsPtr_, __LINE__) = &(cs)

// #define IP_DEFINE_CALLSITE  ---  combine IP_REGISTER_CALLSITE and MakeCallsite to clean up api

namespace instprof {

    struct AggregateStats {

        uint64_t callCount = 0;
        int64_t  totalInclusiveTime = 0; // nanoseconds
        int64_t  totalSelfTime = 0;
        int64_t  maxInclusiveTime = 0;
        int64_t  maxSelfTime = 0;
    };

    struct CallsiteInfo {

        const char* name;
        const char* function;
        const char* file;
        uint32_t line;
        AggregateStats stats{};
    };

    IP_FORCE_INLINE constexpr CallsiteInfo MakeCallsite(const char* name, std::source_location source = std::source_location::current()) noexcept {
        
        return CallsiteInfo{
            .name = name,
            .function = source.function_name(),
            .file = source.file_name(),
            .line = static_cast<uint32_t>(source.line()),
            .stats = {}
        };
    }

}
