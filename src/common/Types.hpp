#pragma once

#include <cstdint>


//Fixed-Width Integer Types
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using s8  = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

//Memory Sizes

//KibiByte
inline constexpr auto operator""_KiB(size_t value) -> size_t {
    return value * 1024;
}

//MebiByte
inline constexpr auto operator""_MiB(size_t value) -> size_t {
    return value * 1024 * 1024;
}