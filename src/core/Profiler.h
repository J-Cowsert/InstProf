#pragma once

#include "Core.h"

#include <stdint.h>
#include <chrono>


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
            return 0;
        }

    private:
        
        static Profiler* s_Instance;

    private:

    };

}
