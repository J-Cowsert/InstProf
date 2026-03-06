#pragma once

#include "Assert.h"
#include <assert.h>

#include <cstddef>
#include <atomic>

namespace instprof {

    template<typename T, size_t Capacity>
    class SPSCQueue
    {
    public:
        SPSCQueue()
            : m_Data(new T[Capacity])
        {
            static_assert(Capacity > 0, "Capacity must be greater than zero");
            static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");
        }
        
        ~SPSCQueue() { delete[] m_Data; }

        bool TryPush(T value) {

            size_t head = m_Head.load();
            size_t next = (head + 1) & (Capacity - 1);
            if (next == m_Tail.load()) return false;
            m_Data[head] = value;
            m_Head.store(next);
            return true;
        }

        
        bool TryPop(T& out) {
            
            size_t tail = m_Tail.load();
            if (tail == m_Head.load()) return false;
            out = m_Data[tail];
            m_Tail.store((tail + 1) & (Capacity - 1));
            return true;
        }

    private:


        T* m_Data;

        // False Sharing
        alignas(64) std::atomic<size_t> m_Head = 0;
        alignas(64) std::atomic<size_t> m_Tail = 0;
    };

}
