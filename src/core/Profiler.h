#pragma once

#include "Core.h"
#include "Log.h"

#include <cstdint>
#include <chrono>

#include <time.h>


/*
    std::source_location
    std::stacktrace
*/

namespace instprof {

    class Profiler {
    public:

        Profiler();
        ~Profiler() = default;
        
        static Profiler& GetProfiler() { return *s_Instance; }
        
        static IP_FORCE_INLINE int64_t GetTime() 
        {
            #if 0 && defined(__x86_64__)
                // need to 
                // ensure invariant TCS
                // calibrate clock frequency
                //

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
