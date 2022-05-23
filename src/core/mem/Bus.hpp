#pragma once

#include "GamePak.hpp"
#include "core/Scheduler.hpp"
#include "core/ppu/PPU.hpp"
#include "common/Types.hpp"

#include <vector>


namespace emu {

//TODO:
// - Sequential and Non-Sequential Accesses
// enum AccessType {
//     SEQUENTIAL, NON_SEQUENTIAL
// };

class Bus {
private:

    struct Memory {
        u8 bios[16_KiB];   //00000000 - 00003FFF
        u8 ewram[256_KiB]; //02000000 - 0203FFFF
        u8 iwram[32_KiB];  //03000000 - 03007FFF
        u8 io[1023];       //04000000 - 040003FE
        //Rest of general memory
    } m_mem;

    GamePak m_pak;
    Scheduler &m_scheduler;
    PPU &m_ppu;

    void wait(u32 cycles); //For waitstates
    auto read_byte(u32 address) -> u8;
    void write_byte(u32 address, u8 value);
    auto read_io(u32 address) -> u8;
    void write_io(u32 address, u8 value);

public:

    Bus(Scheduler &scheduler, PPU &ppu);

    void reset();

    auto read8(u32 address) -> u8;
    auto read16(u32 address) -> u16; 
    auto read32(u32 address) -> u32;
    void write8(u32 address, u8 value);
    void write16(u32 address, u16 value);
    void write32(u32 address, u32 value);
    void cycle();

    auto getLoadedPak() -> GamePak&;
    void loadROM(const std::vector<u8> &rom);
    void loadBIOS(const std::vector<u8> &bios);

    //For Debugger
    auto debugRead8(u32 address) -> u8;
    auto debugRead16(u32 address) -> u16;
    auto debugRead32(u32 address) -> u32;
    void debugWrite8(u32 address, u8 value);
    void debugWrite16(u32 address, u16 value);
    void debugWrite32(u32 address, u32 value);
};

} //namespace emu