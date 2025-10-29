#pragma once

#include "Core.h"

#include <cstdint>
#include <chrono> // TODO: shouldnt include unless its actually used
#include <sys/types.h>
#include <time.h>

#include <unistd.h>
#include <sys/syscall.h>


namespace instprof {

    IP_FORCE_INLINE int64_t GetTime() {

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

    IP_FORCE_INLINE uint32_t GetCurrentThreadID() noexcept {

        #if defined(IP_PLATFORM_LINUX)
            return syscall(SYS_gettid);
        #else
            #error #error Platform not supported
        #endif
    }

    IP_FORCE_INLINE uint32_t GetCurrentProcessID() {

        return 0;
    }

}