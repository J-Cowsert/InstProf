#pragma once

#include "Core.h"
#include "Log.h"
#include "Assert.h"

#include <cstdint>
#include <chrono>
#include <source_location>

#include <time.h>


/*
    std::source_location
    std::stacktrace
*/

namespace instprof {

    struct CallsiteInfo {

        std::string_view name; 
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
        
        static Profiler& GetProfiler() { return *s_Instance; }
        
        static IP_FORCE_INLINE int64_t GetTime() {

            #if 0 && defined(__x86_64__)
                // need to: 
                //   ensure invariant TCS
                //   calibrate clock frequency
                uint64_t rax, rdx;
                asm volatile("rdtsc" : "=a" (rax), "=d" (rdx));
                return (int64_t)(rdx << 32) | rax;
                
            #elif defined(IP_PLATFORM_LINUX) && defined(CLOCK_MONOTONIC_RAW)
                struct timespec ts;
                clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
                return int64_t( ts.tv_sec ) * 1000000000ll + int64_t( ts.tv_nsec );
            #else

                return std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::high_resolution_clock::now().time_since_epoch() ).count();
            #endif

            return 0; // Should be unreachable

        }

    private:
        
        static Profiler* s_Instance;

    private:

    };

}
