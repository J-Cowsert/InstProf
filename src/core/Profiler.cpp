#include "Profiler.h"

namespace instprof {


    Profiler* Profiler::s_Instance = nullptr;


    Profiler::Profiler()
    {

        IP_INFO("Profiler Constructor Called");
        s_Instance = this;
        Log::Init(); 
    }
}
