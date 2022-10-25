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

        m_data[m_head] = value;
        m_head = (m_head + 1) % _capacity;
    }

    void pop() {
        std::lock_guard lock(m_access_mutex);

        if(m_size == 0) {
            return;
        }

        m_tail = (m_tail + 1) % _capacity;
        m_size--;
    }

    auto peek(size_t index) -> T {
        std::lock_guard lock(m_access_mutex);
        return m_data[(m_tail + index) % _capacity];
    }

    auto front() -> T {
        std::lock_guard lock(m_access_mutex);
        return m_data[m_tail];
    }

    auto back() -> T {
        std::lock_guard lock(m_access_mutex);
        return m_data[m_head == 0 ? _capacity - 1 : m_head - 1];
    }

    void pop_array(T *dst, size_t size) {
        std::lock_guard lock(m_access_mutex);
        assert(size <= m_size);

        size_t to_end = _capacity - m_tail;
        if(to_end > size) {
            memcpy(dst, &m_data[m_tail], size * sizeof(T));
        } else {
            size_t second_size = size - to_end;
            memcpy(dst, &m_data[m_tail], to_end * sizeof(T));
            memcpy(dst + to_end, m_data, second_size * sizeof(T));
        }

        m_tail = (m_tail + size) % _capacity;
        m_size -= size;
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