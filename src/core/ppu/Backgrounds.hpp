#pragma once

#include "common/Types.hpp"


namespace emu {

struct TextBackground {
    u16 bgcnt;
    u16 bghofs;
    u16 bgvofs;

    auto getTextPixel(int x, int y, u8 *vram, u8 *palette) -> u32;
};

struct RotScaleBackground : TextBackground {

};

struct BitmapBackground : RotScaleBackground {
    auto getBitmapPixelMode3(int x, int y, u8 *vram) -> u32;
    auto getBitmapPixelMode4(int x, int y, u8 *vram, u8 *palette, bool frame_1) -> u32;
    auto getBitmapPixelMode5(int x, int y, u8 *vram, bool frame_1) -> u32;
};

} //namespace emu