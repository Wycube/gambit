#pragma once

#include "emulator/core/Scheduler.hpp"
#include "emulator/core/Keypad.hpp"
#include "emulator/core/Timer.hpp"
#include "emulator/core/DMA.hpp"
#include "emulator/core/ppu/PPU.hpp"
#include "emulator/core/mem/Bus.hpp"
#include "emulator/core/cpu/CPU.hpp"
#include "emulator/device/VideoDevice.hpp"
#include "emulator/device/InputDevice.hpp"


namespace emu {

class GBA final {
public:

    GBA(VideoDevice &video_device, InputDevice &input_device);

    void reset();

    void step();
    auto run(u32 cycles) -> u32;
    auto getGamePak() -> GamePak&;
    void loadROM(std::vector<u8> &&rom);
    void loadBIOS(const std::vector<u8> &bios);

    auto getVideoDevice() -> VideoDevice&;
    auto getInputDevice() -> InputDevice&;

    auto getDebugger() -> dbg::Debugger&;

    //Temp
    auto getCurrentTimestamp() -> u32 {
        return m_scheduler.getCurrentTimestamp();
    }

private:

    VideoDevice &m_video_device;
    InputDevice &m_input_device;
    Scheduler m_scheduler;
    Keypad m_keypad;
    Timer m_timer;
    DMA m_dma;
    PPU m_ppu;
    Bus m_bus;
    CPU m_cpu;
    dbg::Debugger m_debugger;
};

} //namespace emu