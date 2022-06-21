#pragma once

#include "core/Scheduler.hpp"
#include "core/Keypad.hpp"
#include "core/ppu/PPU.hpp"
#include "core/mem/Bus.hpp"
#include "core/cpu/CPU.hpp"
#include <vector>


namespace emu {

class GBA {
public:

    GBA();

    void reset();

    void step(u32 cycles = 1);
    auto getGamePak() -> GamePak&;
    void loadROM(std::vector<u8> &&rom);
    void loadBIOS(const std::vector<u8> &bios);

    auto getKeypad() -> Keypad&;
    auto getDebugger() -> dbg::Debugger&;

    //Temp
    auto getCurrentTimestamp() -> u32 {
        return m_scheduler.getCurrentTimestamp();
    }

private:

    Scheduler m_scheduler;
    Keypad m_keypad;
    PPU m_ppu;
    Bus m_bus;
    CPU m_cpu;

    dbg::Debugger m_debugger;
};

} //namespace emu