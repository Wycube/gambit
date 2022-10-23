#pragma once

#include <mutex>


//A thread-safe queue implemented as a ring buffer utilizing mutexes
template<typename T, size_t _capacity>
class ThreadSafeQueue {
public:

    ThreadSafeQueue() {
        m_head = 0;
        m_tail = 0;
        memset(m_data, 0, sizeof(m_data));
    }

    void push(T value) {
        std::lock_guard lock(m_access_mutex);

        if(m_size == _capacity) {
            m_tail = (m_tail + 1) % _capacity;
        } else {
            m_size++;
        }

        m_data[m_head++ % _capacity] = value;
    }

    void pop() {
        std::lock_guard lock(m_access_mutex);

        if(m_size == 0) {
            return;
        }

        m_tail = (m_tail + 1) % _capacity;
        m_size--;
    }

    auto peek() -> T {
        std::lock_guard lock(m_access_mutex);
        return m_data[m_tail];
    }

    auto size() -> size_t {
        std::lock_guard lock(m_access_mutex);
        return m_size;
    }

private:

    T m_data[_capacity];
    size_t m_head, m_tail;
    size_t m_size;
    std::mutex m_access_mutex;
};