#pragma once

#include "Assert.h"
#include <assert.h>

#include <cstddef>
#include <cstring>
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

            const size_t head = m_Head.load(std::memory_order_relaxed);
            const size_t next = (head + 1) & (Capacity - 1);
            if (next == m_Tail.load(std::memory_order_acquire)) return false;
            m_Data[head] = value;
            m_Head.store(next, std::memory_order_release);
            return true;
        }
        
        bool TryPop(T& out) {
            
            const size_t tail = m_Tail.load(std::memory_order_relaxed);
            if (tail == m_Head.load(std::memory_order_acquire)) return false;
            out = m_Data[tail];
            m_Tail.store((tail + 1) & (Capacity - 1), std::memory_order_release);
            return true;
        }

        size_t TryPopBatch(T* out, size_t maxCount) {

            if (maxCount == 0) return 0;

            const size_t tail = m_Tail.load(std::memory_order_relaxed);
            const size_t head = m_Head.load(std::memory_order_acquire);

            const size_t available = (head - tail) & (Capacity - 1);
            const size_t count = (available < maxCount) ? available : maxCount;
            if (count == 0) return 0;

            const size_t first = ((Capacity - tail) < count) ? (Capacity - tail) : count;
            std::memcpy(out, m_Data + tail, first * sizeof(T));

            const size_t second = count - first;
            if (second)
                std::memcpy(out + first, m_Data, second * sizeof(T));

            m_Tail.store((tail + count) & (Capacity - 1), std::memory_order_release);

            return count;
        }

    private:
        static_assert(std::is_trivially_copyable_v<T>, "SPSCQueue type must be trivially constructable");
        static_assert(Capacity > 0, "Capacity must be greater than zero");
        static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

        // T* m_Data;
        T m_Data[Capacity];
        
        alignas(64) std::atomic<size_t> m_Head = 0;
        alignas(64) std::atomic<size_t> m_Tail = 0;
    };

}
