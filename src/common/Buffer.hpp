#pragma once

#include <mutex>
#include <cstring>
#include <cassert>


namespace common {

template<typename T, size_t _capacity>
class FixedRingBuffer {
public:

    FixedRingBuffer() {
        pos = 0;
        std::memset(data, 0, sizeof(data));
    }

    inline void push(T value) {
        data[pos] = value;
        pos = (pos + 1) % _capacity;
    }

    inline auto peek(size_t index) const -> T {
        return data[(pos + index) % _capacity];
    }

    inline void copy(T *dst) const {
        std::memcpy(dst, &data[pos], (_capacity - pos) * sizeof(T));
        std::memcpy(dst + (_capacity - pos), data, pos * sizeof(T));
    }

    inline auto front() const -> T {
        return data[pos];
    }

    inline auto back() const -> T {
        return data[pos == 0 ? _capacity - 1 : pos - 1];
    }

    constexpr auto capacity() const -> size_t {
        return _capacity;
    }

private:

    T data[_capacity];
    size_t pos;
};


template<typename T, size_t _capacity>
class RingBuffer {
public:

    RingBuffer() {
        head = 0;
        tail = 0;
        buf_size = 0;
        std::memset(data, 0, sizeof(data));
    }

    inline void push(T value) {
        if(buf_size == _capacity) {
            tail = (tail + 1) % _capacity;
        } else {
            buf_size++;
        }

        data[head] = value;
        head = (head + 1) % _capacity;
    }

    inline void pop() {
        if(buf_size == 0) {
            return;
        }

        tail = (tail + 1) % _capacity;
        buf_size--;
    }

    inline auto peek(size_t index) const -> T {
        return data[(tail + index) % _capacity];
    }

    inline void copy(T *dst) const {
        std::memcpy(dst, &data[tail], (_capacity - tail) * sizeof(T));
        std::memcpy(dst + (_capacity - tail), data, tail * sizeof(T));
    }

    inline void pop_many(T *dst, size_t amount) {
        assert(amount <= buf_size);

        size_t to_end = _capacity - tail;
        if(to_end > amount) {
            std::memcpy(dst, &data[tail], amount * sizeof(T));
        } else {
            size_t second_size = amount - to_end;
            std::memcpy(dst, &data[tail], to_end * sizeof(T));
            std::memcpy(dst + to_end, data, second_size * sizeof(T));
        }

        tail = (tail + amount) % _capacity;
        buf_size -= amount;
    }

    inline auto front() const -> T {
        return data[tail];
    }

    inline auto back() const -> T {
        assert(buf_size != 0);

        return data[head == 0 ? _capacity - 1 : head - 1];
    }

    inline auto size() const -> size_t {
        return buf_size;
    }

    inline auto capacity() const -> size_t {
        return _capacity;
    }

private:

    T data[_capacity];
    size_t head, tail, buf_size;
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