#pragma once

#include "Types.hpp"
#include <type_traits>
#include <cassert>


namespace bits {

template<u8 bit, typename T>
constexpr auto get_bit(T value) -> bool {
    static_assert(std::is_integral_v<T>);
    static_assert(bit < sizeof(T) * 8);

    return value >> bit & 1;
}

template<typename T>
constexpr auto get_bit(T value, u8 bit) -> bool {
    // assert(std::is_integral_v<T>);
    // assert(bit < sizeof(T) * 8);


    return value >> bit & 1;
}

template<u8 start, u8 size, typename T>
constexpr auto get(T value) -> T {
    static_assert(std::is_integral_v<T>);
    static_assert((start + size) <= sizeof(T) * 8);

    return value >> start & ((static_cast<T>(1) << size) - 1);
}

template<u8 start, u8 size, typename T, typename U>
constexpr void set(T &value, U other) {
    static_assert(std::is_integral_v<T>);
    static_assert(std::is_integral_v<U>);
    static_assert((start + size) <= sizeof(T) * 8);
    static_assert(size <= sizeof(U) * 8);

    T mask = (static_cast<T>(1) << size) - 1;
    value = (value & ~(mask << start)) | ((other & mask) << start);
}

template<typename T>
constexpr auto get(u8 start, u8 size, T value) -> T {
    static_assert(std::is_integral_v<T>);
    // assert((start + size) <= sizeof(T) * 8);

    return value >> start & ((static_cast<T>(1) << size) - 1);
}

template<typename T, typename U>
constexpr void set(u8 start, u8 size, T &value, U other) {
    static_assert(std::is_integral_v<T>);
    static_assert(std::is_integral_v<U>);
    // assert((start + size) <= sizeof(T) * 8);
    // assert(size <= sizeof(U) * 8);

    T mask = (static_cast<T>(1) << size) - 1;
    value = (value & ~(mask << start)) | ((other & mask) << start);
}

template<typename T>
constexpr auto popcount(T value) -> u8 {
    static_assert(std::is_integral_v<T>);
    
    u8 count = 0;

    for(; value != 0; value &= value - 1) {
        count++;
    }

    return count;
}

template<u8 start_size, typename R, typename T>
constexpr auto sign_extend(T value) -> R {
    static_assert(std::is_integral_v<T>);
    static_assert(start_size <= sizeof(T) * 8);
    static_assert(start_size < sizeof(R) * 8);
    static_assert(sizeof(T) <= sizeof(R));

    return static_cast<R>(value) | ((value >> (start_size - 1)) & 0x1 ? ~((static_cast<R>(1) << (start_size - 1)) - 1) : 0);
}

template<typename A, typename T>
constexpr auto align(T value) -> T {
    static_assert(std::is_integral_v<A>);
    static_assert(std::is_integral_v<T>);

    return value & ~(sizeof(A) - 1);
}

constexpr auto lsl(u32 value, u8 shift) -> u32 {
    return shift > 31 ? 0 : value << shift;
}

constexpr auto lsl_c(u32 value, u8 shift, bool &carry) -> u32 {
    if(shift == 0) {
        return value;
    }

    carry = shift > 32 ? false : get_bit(value, 32 - shift);
    return lsl(value, shift);
}

constexpr auto lsr(u32 value, u8 shift) -> u32 {
    return shift > 31 ? 0 : value >> shift;
}

constexpr auto lsr_c(u32 value, u8 shift, bool &carry) -> u32 {
    if(shift == 0) {
        return value;
    }

    carry = shift > 32 ? false : get_bit(value, shift - 1);
    return lsr(value, shift);
}

constexpr auto asr(u32 value, u8 shift) -> u32 {
    return (s32)value >> (shift > 31 ? 31 : shift);
}

constexpr auto asr_c(u32 value, u8 shift, bool &carry, bool i = false) -> u32 {
    if(shift == 0) {
        return value;
    }

    carry = get_bit(value, (shift > 32 ? 32 : shift) - 1);
    return bits::asr(value, shift);
}

constexpr auto ror(u32 value, u8 rotate) -> u32 {
    return (value >> rotate) | (value << (32 - rotate));
}

constexpr auto ror_c(u32 value, u8 rotate, bool &carry) {
    u32 result = ror(value, rotate);
    carry = get_bit<31>(result);
    return result;
}

constexpr auto rrx(u32 value, bool carry) -> u32 {
    return (carry << 31) | (value >> 1);
}

constexpr auto rrx_c(u32 value, bool &carry) -> u32 {
    bool old_carry = carry;
    carry = get_bit<0>(value);
    return rrx(value, old_carry);
}

} //namespace common