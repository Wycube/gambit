#pragma once

#include "core/Scheduler.hpp"
#include "core/mem/Bus.hpp"
#include "core/debug/Debugger.hpp"
#include "Backgrounds.hpp"


namespace emu {

enum LCDState : u8 {
    VBLANK = 0,
    HBLANK = 1,
    DRAWING = 2
};

class PPU {
public:

    PPU(Scheduler &scheduler, Bus &bus);

    void reset();

    void run(u32 current, u32 late);

    auto readIO(u32 address) -> u8;
    auto readPalette(u32 address) -> u8;
    auto readVRAM(u32 address) -> u8;
    auto readOAM(u32 address) -> u8;
    void writeIO(u32 address, u8 value);
    void writePalette(u32 address, u8 value);
    void writeVRAM(u32 address, u8 value);
    void writeOAM(u32 address, u8 value);

    void attachDebugger(dbg::Debugger &debugger);

private:

    LCDState m_state;
    u8 m_line;
    u16 m_dot;

    //Memory
    u16 m_dispcnt;
    u16 m_dispstat;
    u8 m_palette[1_KiB];
    u8 m_vram[96_KiB];
    u8 m_oam[1_KiB];

    //Backgrounds
    TextBackground m_bg0;
    TextBackground m_bg1;
    BitmapBackground m_bg2;
    RotScaleBackground m_bg3;

    u32 m_internal_framebuffer[240 * 160];
    u32 m_present_framebuffer[240 * 160];

    Scheduler &m_scheduler;
    Bus &m_bus;

    auto _readIO(u32 address) -> u8;
    void _writeIO(u32 address, u8 value);

    void hblankStart(u32 current, u32 late);
    void hblankEnd(u32 current, u32 late);
    void vblank(u32 current, u32 late);

    void writeLineMode0();
    void writeLineMode3();
    void writeLineMode4();
    void writeLineMode5();
};

} //namespace emu