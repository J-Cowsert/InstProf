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
        SPSCQueue() = default;

        SPSCQueue(const SPSCQueue&) = delete;
        SPSCQueue& operator=(const SPSCQueue&) = delete;
        SPSCQueue(SPSCQueue&&) = delete;
        SPSCQueue& operator=(SPSCQueue&&) = delete;

        bool TryPush(T value) {

            size_t head = m_Head.load(std::memory_order_relaxed);
            size_t next = (head + 1) & (Capacity - 1);
            if (next == m_Tail.load(std::memory_order_acquire)) return false;
            m_Data[head] = value;
            m_Head.store(next, std::memory_order_release);
            return true;
        }
        
        bool TryPop(T& out) {
            
            size_t tail = m_Tail.load(std::memory_order_relaxed);
            if (tail == m_Head.load(std::memory_order_acquire)) return false;
            out = m_Data[tail];
            m_Tail.store((tail + 1) & (Capacity - 1), std::memory_order_release);
            return true;
        }

    private:
        static_assert(Capacity > 0, "Capacity must be greater than zero");
        static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

        // T* m_Data;
        T m_Data[Capacity];
        
        alignas(64) std::atomic<size_t> m_Head = 0;
        alignas(64) std::atomic<size_t> m_Tail = 0;
    };

}
