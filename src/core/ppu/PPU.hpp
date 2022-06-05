#pragma once

#include "core/Scheduler.hpp"
#include "core/debug/Debugger.hpp"


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
    u16 m_dispcnt;
    u16 m_dispstat;
    u8 m_palette[1_KiB]; 
    u8 m_vram[96_KiB]; 
    u8 m_oam[1_KiB]; 

    u32 m_framebuffer[240 * 160];

    Scheduler &m_scheduler;

    auto read_io(u32 address) -> u8;
    void write_io(u32 address, u8 value);

public:

    PPU(Scheduler &scheduler);

    void reset();

    void run(u32 current, u32 late);
    void writeFrameMode3();
    void writeFrameMode4();

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

    void attachDebugger(dbg::Debugger &debugger);
};

} //namespace emu