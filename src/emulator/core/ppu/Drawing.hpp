#pragma once

#include "common/Types.hpp"


namespace emu {

struct Background {
    u16 control;
    u16 h_offset, v_offset;

    //Rotation/Scaling Registers
    u32 reference_x, reference_y;
    u16 param_a = 0x10, param_b, param_c = 0x10, param_d;


    auto getTextPixel(int x, int y, const u8 *vram) -> u8;
    
    void updateAffineParams();
    void reloadInternalRegs();
    void incrementInternalRegs();
    auto getAffinePixel(int x, int y, const u8 *vram) -> u8;

    auto getBitmapPixelMode3(int x, int y, const u8 *vram) -> u16;
    auto getBitmapPixelMode4(int x, int y, const u8 *vram, const u8 *palette, bool frame_1) -> u16;
    auto getBitmapPixelMode5(int x, int y, const u8 *vram, bool frame_1) -> u16;

    private:

    //Switch to fixed-point later on
    float _x, _y;
    float _a, _b, _c, _d;
};

struct Window {
    u16 win0h, win1h;
    u16 win0v, win1v;
    u16 winin, winout;

    auto insideWindow0(int x, int y) -> bool;
    auto insideWindow1(int x, int y) -> bool;
};

} //namespace emu