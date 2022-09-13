#pragma once

#include "emulator/core/Scheduler.hpp"
#include "emulator/core/mem/Bus.hpp"
#include "emulator/core/DMA.hpp"
#include "emulator/device/VideoDevice.hpp"
#include "Types.hpp"


namespace emu {

class PPU final {
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

    PPUState m_state;
    u8 m_win_line[240];
    u16 m_bmp_col[240];
    u8 m_bg_col[4][240];
    u8 m_obj_col[240];
    u8 m_obj_prios[240];

    VideoDevice &m_video_device;
    Scheduler &m_scheduler;
    Bus &m_bus;
    DMA &m_dma;

    void hblankStart(u64 current, u64 late);
    void hblankEnd(u64 current, u64 late);

    void clearBuffers();
    void getWindowLine();
    void drawObjects();
    void drawBackground();
    void compositeLine();
};

} //namespace emu