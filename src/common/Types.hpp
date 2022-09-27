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


//KibiByte
inline constexpr auto operator""_KiB(size_t value) -> size_t {
    return value * 1024;
}

//MebiByte
inline constexpr auto operator""_MiB(size_t value) -> size_t {
    return value * 1024 * 1024;
}


// template<int integral, int fractional, typename T, typename D>
// struct Fixed {
//     static_assert(std::is_integral_v<T>);
//     static_assert(std::is_integral_v<T>);
//     static_assert(integral + fractional <= sizeof(T) * 8);

//     T raw;

//     constexpr auto integral() -> T {
//         return (raw >> fractional) & ~((1 << integral) - 1);
//     }

//     constexpr auto fractional() -> T {
//         return raw & ~((1 << fractional) - 1);
//     }

//     constexpr auto operator +(const Fixed &other) -> Fixed {
//         return Fixed{raw + other.raw};
//     }

//     constexpr auto operator -(const Fixed &other) -> Fixed {
//         return Fixed{raw - other.raw};
//     }

//     constexpr auto operator *(const Fixed &other) -> Fixed {
//         D double_size = ((D)raw << fractional) * ((D)other.raw << fractional);
//         return (T)(double_size >> fractional);
//     }

//     constexpr auto operator /(const Fixed &other) -> Fixed {

//     }
// };