#pragma once

#include "GamePak.hpp"
#include "common/Types.hpp"

#include <vector>


namespace emu {

class Bus {
private:

    struct Memory {
        u8 bios[16_KiB];   //00000000 - 00003FFF
        u8 ewram[256_KiB]; //02000000 - 0203FFFF
        u8 iwram[32_KiB];  //03000000 - 03007FFF
        //Rest of general memory
    } m_mem;

    GamePak m_pak;

public:

    Bus();

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);
    auto read16(u32 address) -> u16; 
    void write16(u32 address, u16 value);
    auto read32(u32 address) -> u32;
    void write32(u32 address, u32 value);

    void loadROM(const std::vector<u8> &rom);
};

} //namespace emu