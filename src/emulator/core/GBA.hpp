#pragma once

#include "emulator/core/Scheduler.hpp"
#include "emulator/core/debug/DebugInterface.hpp"
#include "emulator/core/Keypad.hpp"
#include "emulator/core/Timer.hpp"
#include "emulator/core/DMA.hpp"
#include "emulator/core/SIO.hpp"
#include "emulator/core/ppu/PPU.hpp"
#include "emulator/core/apu/APU.hpp"
#include "emulator/core/mem/Bus.hpp"
#include "emulator/core/cpu/CPU.hpp"
#include "emulator/device/VideoDevice.hpp"
#include "emulator/device/InputDevice.hpp"
#include "emulator/device/AudioDevice.hpp"


namespace emu {

class GBA final {
public:

    GBA(VideoDevice &video_device, InputDevice &input_device, AudioDevice &audio_device);

    void reset(bool skip_bios = true, bool enable_debugger = false);
    void step();
    auto run(u32 cycles) -> u32;
    void loadBIOS(const std::vector<u8> &bios);

    VideoDevice &video_device;
    InputDevice &input_device;
    AudioDevice &audio_device;
    Scheduler scheduler;
    DebugInterface debug;
    Keypad keypad;
    Timer timer;
    DMA dma;
    SIO sio;
    PPU ppu;
    APU apu;
    Bus bus;
    CPU cpu;

    u32 cycles_active = 0;
    bool enable_debugger = false;
};

} //namespace emu