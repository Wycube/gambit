#pragma once

#include "emulator/core/Scheduler.hpp"
#include "emulator/core/mem/Bus.hpp"
#include "emulator/device/VideoDevice.hpp"
#include "Backgrounds.hpp"


namespace emu {

class PPU {
public:

    PPU(VideoDevice &video_device, Scheduler &scheduler, Bus &bus);

    void reset();

    //void run(u32 current, u32 late);

    auto readIO(u32 address) -> u8;
    auto readPalette(u32 address) -> u8;
    auto readVRAM(u32 address) -> u8;
    auto readOAM(u32 address) -> u8;
    void writeIO(u32 address, u8 value);
    void writePalette(u32 address, u8 value);
    void writeVRAM(u32 address, u8 value);
    void writeOAM(u32 address, u8 value);

private:

    u8 m_line;

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

    VideoDevice &m_video_device;
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