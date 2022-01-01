#pragma once

#include "common/Types.hpp"

#include <vector>


namespace emu {

class GamePak {
private:

    std::vector<u8> m_rom;

public:

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

    void loadROM(const std::vector<u8> &rom);
};

} //namespace emu