#pragma once

#include "core/cpu/CPU.hpp"
#include "core/mem/Bus.hpp"

#include <vector>


namespace emu {

class GBA {
private:

    CPU m_cpu;
    Bus m_bus;

public:

    GBA();

    void loadROM(std::vector<u8> &rom);
};

} //namespace emu