#include <thread>

#include "core/Profiler.h"
#include "core/Log.h"

int main()
{
    auto start = instprof::Profiler::GetTime();
    {
    }
    auto end = instprof::Profiler::GetTime();
    
    IP_INFO("End-Start Time: {} ns", (end - start));

    
    IP_INFO("End Time: {}", end);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    auto curr = instprof::Profiler::GetTime();

    IP_INFO("Curr Time: {}", curr); 
    IP_INFO("curr-end Time: {} ns", (curr - end));
}
