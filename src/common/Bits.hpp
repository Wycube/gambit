#pragma once

#include "Types.hpp"

#include <cassert>


namespace bits {

template<u8 start, u8 size, typename T>
inline auto constexpr get(T value) -> T {
    static_assert((start + size) <= sizeof(value) * 8);

    return value >> start & ((static_cast<T>(1) << size) - 1);
}

template<typename T>
inline auto constexpr get(u8 start, u8 size, T value) -> T {
    assert((start + size) <= sizeof(value) * 8);

    return value >> start & ((static_cast<T>(1) << size) - 1);
}

inline auto ror(u32 value, u8 rotate) -> u32 {
    return (value >> rotate) | (value << (32 - rotate));
}

inline auto asr(u32 value, u8 shift) -> u32 {
    return (s32)value >> shift;
}

inline auto popcount_16(u16 value) -> u8 {
    u8 count = 0;
    for(; value != 0; value &= value - 1) {
        count++;
    }

    return count;
}

template<u8 start_size, typename R, typename T>
inline auto sign_extend(T value) -> R {
    static_assert(start_size <= sizeof(T) * 8);
    static_assert(start_size < sizeof(R) * 8);
    static_assert(sizeof(T) <= sizeof(R));

    return static_cast<R>(value) | ((value >> (start_size - 1)) & 0x1 ? ~((static_cast<R>(1) << (start_size - 1)) - 1) : 0);
}

} //namespace common