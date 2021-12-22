#pragma once

#include "GamePak.hpp"
#include "common/Types.hpp"

#include <vector>


namespace emu {

class Bus {
private:

    struct Memory {
        u8 bios[16_KiB];   //00000000 - 00003FFF
        u8 wram1[256_KiB]; //02000000 - 0203FFFF
        u8 wram2[32_KiB];  //03000000 - 03007FFF
        //Rest of general memory
    } m_mem;

    GamePak m_pak;

public:

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

    void loadROM(std::vector<u8> &rom);
};

} //namespace emu