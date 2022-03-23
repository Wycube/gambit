#pragma once

#include "GamePak.hpp"
#include "common/Types.hpp"

#include <vector>


namespace emu {

//TODO:
// - Sequential and Non-Sequential Accesses

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
    auto read16(u32 address) -> u16; 
    auto read32(u32 address) -> u32;
    void write8(u32 address, u8 value);
    void write16(u32 address, u16 value);
    void write32(u32 address, u32 value);

    auto romSize() -> u32;
    void loadROM(const std::vector<u8> &rom);

    //For Debugger
    auto debugRead8(u32 address) -> u8;
    auto debugRead16(u32 address) -> u16;
    auto debugRead32(u32 address) -> u32;
    void debugWrite8(u32 address, u8 value);
    void debugWrite16(u32 address, u16 value);
    void debugWrite32(u32 address, u32 value);
};

} //namespace emu