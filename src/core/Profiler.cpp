#include "Profiler.h"

#include "core/System.h"

namespace instprof {

    Profiler::Profiler() 
        : m_MainThreadID(GetCurrentThreadID())
    {
        
    }

}
