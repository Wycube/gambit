#pragma once

#include "emulator/core/Scheduler.hpp"
#include "emulator/core/mem/Bus.hpp"
#include "emulator/core/DMA.hpp"
#include "emulator/device/VideoDevice.hpp"
#include "Types.hpp"


namespace emu {

class PPU {
public:

    PPU(VideoDevice &video_device, Scheduler &scheduler, Bus &bus, DMA &dma);

    void reset();

    auto readIO(u32 address) -> u8;
    void writeIO(u32 address, u8 value);

    template<typename T>
    auto readPalette(u32 address) -> T;
    template<typename T>
    auto readVRAM(u32 address) -> T;
    template<typename T>
    auto readOAM(u32 address) -> T;
    template<typename T>
    void writePalette(u32 address, T value);
    template<typename T>
    void writeVRAM(u32 address, T value);
    template<typename T>
    void writeOAM(u32 address, T value);

    void attachDebugger(dbg::Debugger &debugger);

private:

    // u8 m_line;

    // //Memory
    // u16 m_dispcnt;
    // u16 m_dispstat;
    // u8 m_palette[1_KiB];
    // u8 m_vram[96_KiB];
    // u8 m_oam[1_KiB];

    // Background m_bg[4];
    // Window m_win;
    PPUState m_state;

    VideoDevice &m_video_device;
    Scheduler &m_scheduler;
    Bus &m_bus;
    DMA &m_dma;

    auto _readIO(u32 address) -> u8;
    void _writeIO(u32 address, u8 value);

    void hblankStart(u32 current, u32 late);
    void hblankEnd(u32 current, u32 late);
    void vblank(u32 current, u32 late);

    void writeLineMode0();
    void writeLineMode1();
    void writeLineMode3();
    void writeLineMode4();
    void writeLineMode5();
};

} //namespace emu