#pragma once

#include "Core.h"
#include "Log.h"
#include "TSRingBuffer.h"
#include "Event.h"

#include <cstdint>

/*
    Look into std::stacktrace
*/

namespace instprof {

    class Profiler {
    public:

        Profiler();
        ~Profiler();

        static Profiler& Get() {
            
            static Profiler instance;  
            return instance;
        }

        void EnqueueEvent(const EventItem& e);

    private:
        void DebugDrainQueue();
        
    private:
        uint32_t m_MainThreadID;
        TSRingBuffer<EventItem> m_EventQueue{1024}; // Memory used: 1024 * sizeof(EventItem)
    };

}
