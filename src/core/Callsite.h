#pragma once

#include "core/Core.h"

#include <cstdint>
#include <source_location>

#define IP__REGISTER_CALLSITE(cs) static __attribute__((section("instprof_cs"), used)) instprof::CallsiteInfo* IP_CONCAT(_ipCsPtr_, __LINE__) = &(cs)

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

    IP_FORCE_INLINE constexpr CallsiteInfo MakeCallsite(const char* name, std::source_location loc = std::source_location::current()) noexcept {

        return {
            name,
            loc.function_name(),
            loc.file_name(),
            static_cast<uint32_t>(loc.line()),
            {}
        };
    }

}
