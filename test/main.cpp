#include <thread>

//#include "core/Profiler.h"
//#include "core/Log.h"

#include "instprof.h"


void test_func() {

    IP_FUNC_SCOPE();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

int main()
{
    
    IP_FUNC_SCOPE();

    
    {
        IP_NAMED_SCOPE("Empty Zone");
    }
    
    test_func(); 
}
