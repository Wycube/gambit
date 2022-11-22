#pragma once

#include <mutex>
#include <cstring>
#include <cassert>


namespace common {

template<typename T, size_t _capacity>
class FixedRingBuffer {
public:

    FixedRingBuffer() {
        m_pos = 0;
        std::memset(m_data, 0, sizeof(m_data));
    }

    inline void push(T value) {
        m_data[m_pos] = value;
        m_pos = (m_pos + 1) % _capacity;
    }

    inline auto peek(size_t index) const -> T {
        return m_data[(m_pos + index) % _capacity];
    }

    inline void copy(T *dst) const {
        std::memcpy(dst, &m_data[m_pos], (_capacity - m_pos) * sizeof(T));
        std::memcpy(dst + (_capacity - m_pos), m_data, m_pos * sizeof(T));
    }

    inline auto front() const -> T {
        return m_data[m_pos];
    }

    inline auto back() const -> T {
        return m_data[m_pos == 0 ? _capacity - 1 : m_pos - 1];
    }

    constexpr auto capacity() const -> size_t {
        return _capacity;
    }

private:

    T m_data[_capacity];
    size_t m_pos;
};


template<typename T, size_t _capacity>
class RingBuffer {
public:

    RingBuffer() {
        m_head = 0;
        m_tail = 0;
        m_size = 0;
        std::memset(m_data, 0, sizeof(m_data));
    }

    inline void push(T value) {
        if(m_size == _capacity) {
            m_tail = (m_tail + 1) % _capacity;
        } else {
            m_size++;
        }

        m_data[m_head] = value;
        m_head = (m_head + 1) % _capacity;
    }

    inline void pop() {
        if(m_size == 0) {
            return;
        }

        m_tail = (m_tail + 1) % _capacity;
        m_size--;
    }

    inline auto peek(size_t index) const -> T {
        return m_data[(m_tail + index) % _capacity];
    }

    inline void copy(T *dst) const {
        std::memcpy(dst, &m_data[m_tail], (_capacity - m_tail) * sizeof(T));
        std::memcpy(dst + (_capacity - m_tail), m_data, m_tail * sizeof(T));
    }

    inline void pop_many(T *dst, size_t size) {
        assert(size <= m_size);

        size_t to_end = _capacity - m_tail;
        if(to_end > size) {
            std::memcpy(dst, &m_data[m_tail], size * sizeof(T));
        } else {
            size_t second_size = size - to_end;
            std::memcpy(dst, &m_data[m_tail], to_end * sizeof(T));
            std::memcpy(dst + to_end, m_data, second_size * sizeof(T));
        }

        m_tail = (m_tail + size) % _capacity;
        m_size -= size;
    }

    inline auto front() const -> T {
        return m_data[m_tail];
    }

    inline auto back() const -> T {
        assert(m_size != 0);

        return m_data[m_head == 0 ? _capacity - 1 : m_head - 1];
    }

    inline auto size() const -> size_t {
        return m_size;
    }

    inline auto capacity() const -> size_t {
        return _capacity;
    }

private:

    T m_data[_capacity];
    size_t m_head, m_tail;
    size_t m_size;
};


//A thread-safe queue implemented as a ring buffer utilizing mutexes
template<typename T, size_t _capacity>
class ThreadSafeRingBuffer {
public:

    void push(T value) {
        std::lock_guard lock(m_access_mutex);
        m_buffer.push(value);
    }

    void pop() {
        std::lock_guard lock(m_access_mutex);
        m_buffer.pop();
    }

    auto peek(size_t index) const -> T {
        std::lock_guard lock(m_access_mutex);
        return m_buffer.peek(index);
    }

    void copy(T *dst) const {
        std::lock_guard lock(m_access_mutex);
        m_buffer.copy(dst);
    }

    void pop_many(T *dst, size_t size) {
        std::lock_guard lock(m_access_mutex);
        m_buffer.pop_many(dst, size);
    }

    auto front() const -> T {
        std::lock_guard lock(m_access_mutex);
        return m_buffer.front();
    }

    auto back() const -> T {
        std::lock_guard lock(m_access_mutex);
        return m_buffer.back();
    }

    auto size() const -> size_t {
        std::lock_guard lock(m_access_mutex);
        return m_buffer.size();
    }

    auto capacity() const -> size_t {
        return _capacity;
    }

private:

    RingBuffer<T, _capacity> m_buffer;
    mutable std::mutex m_access_mutex;
};

} //namespace common