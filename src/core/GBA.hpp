#pragma once

#include "core/Scheduler.hpp"
#include "core/ppu/PPU.hpp"
#include "core/mem/Bus.hpp"
#include "core/cpu/CPU.hpp"

#include <vector>


namespace emu {

class GBA {
private:

    Scheduler m_scheduler;
    PPU m_ppu;
    Bus m_bus;
    CPU m_cpu;

    dbg::Debugger m_debugger;

public:

    GBA();

    void reset();

    void step(u32 cycles = 1);
    auto getGamePak() -> GamePak&;
    void loadROM(const std::vector<u8> &rom);
    void loadBIOS(const std::vector<u8> &bios);

    auto getDebugger() -> dbg::Debugger&;
};

} //namespace emu