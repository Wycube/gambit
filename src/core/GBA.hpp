#pragma once

#include "core/cpu/CPU.hpp"
#include "core/mem/Bus.hpp"

#include <vector>


namespace emu {

class GBA {
private:

    CPU m_cpu;
    Bus m_bus;

    dbg::Debugger m_debugger;

public:

    GBA();

    void step();
    void loadROM(const std::vector<u8> &rom);

    auto getDebugger() -> dbg::Debugger&;
};

} //namespace emu