#include <iostream>
#include <thread>

#include "core/Profiler.h"
#include "core/Log.h"

int main()
{
    auto start = instprof::Profiler::GetProfiler().GetTime();
    {
    }
    auto end = instprof::Profiler::GetProfiler().GetTime();
    
    IP_INFO("End-Start Time: {} ms", (end - start));
}
