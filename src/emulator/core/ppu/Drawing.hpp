#pragma once

#include "common/Types.hpp"


namespace emu {

enum BackgroundType {
    REGULAR,
    AFFINE,
    BITMAP
};

struct Background {
    u8 priority;
    u8 char_base_block;
    bool mosaic;
    bool color_mode;
    u8 scr_base_block;
    bool disp_overflow;
    u8 screen_size;
    u8 unused;
    u16 h_offset, v_offset;

    //Rotation/Scaling Registers
    u32 reference_x, reference_y;
    u16 param_a = 0x10, param_b, param_c = 0x10, param_d;

    void write(u32 address, u8 value);
    auto read(u32 address, bool regular) -> u8;

    auto getTextPixel(int x, int y, const u8 *vram) -> u8;
    auto getAffinePixel(int x, int y, const u8 *vram) -> u8;

    auto getBitmapPixelMode3(int x, int y, const u8 *vram) -> u16;
    auto getBitmapPixelMode4(int x, int y, const u8 *vram, const u8 *palette, bool frame_1) -> u16;
    auto getBitmapPixelMode5(int x, int y, const u8 *vram, bool frame_1) -> u16;

    u8 last_scanline = 0;

    private:

    //Switch to fixed-point later on
    // float _x, _y;
    // float _a, _b, _c, _d;
};

struct Window {
    u16 winh[2];
    u16 winv[2];
    u16 winin, winout;

    auto insideWindow(int x, int y, int window) -> bool;
};

} //namespace emu