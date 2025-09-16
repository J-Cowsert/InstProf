#include "Profiler.h"

namespace instprof {


    Profiler* Profiler::s_Instance = nullptr;


    Profiler::Profiler()
    {
        s_Instance = this;
        Log::Init(); 
    }
}
