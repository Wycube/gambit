#pragma once

#include "Types.hpp"


namespace bits {

template<u32 start, u32 size, typename T>
inline auto constexpr get(T value) -> T {
    static_assert((start + size) <= sizeof(value) * 8);

    return value >> start & (((u64)1 << size) - 1);
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

inline auto sign_extend32(u32 value) -> u64 {
    return value | ((value >> 31) & 0x1 ? 0xFFFFFFFF00000000 : 0);
}

inline auto sign_extend16(u16 value) -> u32 {
    return value | ((value >> 15) & 0x1 ? 0xFFFF0000 : 0);
}

inline auto sign_extend8(u8 value) -> u32 {
    return value | ((value >> 7) & 0x1 ? 0xFFFFFF00 : 0);
}

} //namespace common