#pragma once 

#include "TSRingBuffer.h"
#include "Event.h"

#include <atomic>

namespace instprof {

    class TraceEventWorker {
    public:
        TraceEventWorker(TSRingBuffer<EventItem>& q, const char* path)
            : m_Q(q), m_Path(path)
        {
        }

        ~TraceEventWorker() { Stop(); }

        bool Start();
        void Stop();

    private:
        void Comma() { if (!m_First) std::fputs(",\n", m_File); else m_First = false; }
        void Run();

    private:
        std::thread m_Thread;
        TSRingBuffer<EventItem>& m_Q;
        std::FILE* m_File = nullptr;
        
        std::atomic<bool> m_Stop{false}, m_Running{false};
        std::string m_Path;
        bool m_First = true;
    };

}