#pragma once

#include "emulator/core/Scheduler.hpp"
#include "emulator/core/debug/Debugger.hpp"
#include "emulator/core/Keypad.hpp"
#include "emulator/core/Timer.hpp"
#include "emulator/core/DMA.hpp"
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

    void reset();

    void step();
    auto run(u32 cycles) -> u32;
    auto getGamePak() -> GamePak&;
    void loadROM(std::vector<u8> &&rom);
    void loadBIOS(const std::vector<u8> &bios);
    void loadSave(const std::string &filename);
    void writeSave(const std::string &filename);

    VideoDevice &video_device;
    InputDevice &input_device;
    AudioDevice &audio_device;
    Scheduler scheduler;
    dbg::Debugger debugger;
    Keypad keypad;
    Timer timer;
    DMA dma;
    PPU ppu;
    APU apu;
    Bus bus;
    CPU cpu;
};

} //namespace emu