#pragma once

#include "core/Scheduler.hpp"


namespace emu {

enum LCDState : u8 {
    VBLANK = 0,
    HBLANK = 1,
    DRAWING = 2
};

class PPU {
private:

    LCDState m_state;
    u32 m_line;
    u32 m_dot;

    //Memory
    u8 m_palette[1_KiB]; 
    u8 m_vram[96_KiB]; 
    u8 m_oam[1_KiB]; 

    u32 m_framebuffer[240 * 160];

    Scheduler &m_scheduler;

public:

    PPU(Scheduler &scheduler);

    void run(u32 current, u32 late);

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);
};

} //namespace emu