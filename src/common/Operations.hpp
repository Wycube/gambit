#pragma once

#include "Types.hpp"


namespace common {

inline auto ror(u32 value, u8 rotate) -> u32 {
    return (value >> rotate) | (value << (32 - rotate));
}

inline auto asr(u32 value, u8 shift) -> u32 {
    return ((value >> shift) & ~(1 << 31)) | (value & (1 << 31));
}

} //namespace common