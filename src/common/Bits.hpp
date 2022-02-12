#pragma once

#include "Types.hpp"


namespace common {

inline auto ror(u32 value, u8 rotate) -> u32 {
    return (value >> rotate) | (value << (32 - rotate));
}

inline auto asr(u32 value, u8 shift) -> u32 {
    return ((value >> shift) & ~(1 << 31)) | (value & (1 << 31));
}

inline auto popcount_16(u16 value) -> u8 {
    u8 count = 0;
    for(; value != 0; value &= value - 1) {
        count++;
    }

    return count;
}

} //namespace common