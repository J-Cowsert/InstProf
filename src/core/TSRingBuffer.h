#pragma once

#include "core/Assert.h"

#include <cstddef>
#include <vector> 
#include <mutex>
#include <condition_variable>


// Keep simple for now
namespace instprof {

    template<typename T>
    class TSRingBuffer {
    public:
        explicit TSRingBuffer(size_t capacity)
            : m_Buf(capacity)
        {
            IP_ASSERT(capacity != 0, "Capacity must be greater than 0");
        }

        void Push(T v) {

            std::unique_lock<std::mutex> lock(m_Mtx);
            m_NotFull.wait(lock, [&]{ return m_Size < m_Buf.size(); });
            m_Buf[m_Tail] = std::move(v);
            m_Tail = (m_Tail + 1) % m_Buf.size();
            m_Size++;
            lock.unlock();
            m_NotEmpty.notify_one();
        }

        T Pop() {

            std::unique_lock<std::mutex> lock(m_Mtx);
            m_NotEmpty.wait(lock, [&]{ return m_Size > 0; });
            T v = std::move(m_Buf[m_Head]);
            m_Head = (m_Head + 1) % m_Buf.size();
            m_Size--;
            lock.unlock();
            m_NotFull.notify_one();
            return v;
        }

        bool TryPop(T& out) {
            
            std::unique_lock<std::mutex> lock(m_Mtx);
            if (m_Size == 0) return false; 
            out = std::move(m_Buf[m_Head]);
            m_Head = (m_Head + 1) % m_Buf.size();
            m_Size--;
            lock.unlock();
            m_NotFull.notify_one();
            return true;
        }

    private:

        std::vector<T> m_Buf;
        size_t m_Head = 0, m_Tail = 0, m_Size = 0;
        std::mutex m_Mtx;
        std::condition_variable m_NotEmpty, m_NotFull;
    };

}
