#pragma once

#include "Drawing.hpp"


namespace emu {

struct PPUState {
    u8 line;
    u16 dispcnt;
    u16 dispstat;
    u16 bldcnt;
    u16 bldalpha;
    u8 palette[1_KiB];
    u8 vram[96_KiB];
    u8 oam[1_KiB];
    Background bg[4];
    Window win;
};

} //namespace emu