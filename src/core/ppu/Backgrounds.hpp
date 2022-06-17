#pragma once

#include "common/Types.hpp"


namespace emu {

struct Background0 {
    u16 bg0cnt;
    u16 bg0hofs;
    u16 bg0vofs;

    auto getPixelColor(int x, int y, u8 *vram, u8 *palette) -> u32;
};

struct Background1 {
    u16 bg1cnt;
    u16 bg1hofs;
    u16 bg1vofs;

};

struct Background2 {
    u16 bg2cnt;
    u16 bg2hofs;
    u16 bg2vofs;

};

struct Background3 {
    u16 bg3cnt;
    u16 bg3hofs;
    u16 bg3vofs;

};

} //namespace emu