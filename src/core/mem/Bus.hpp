#pragma once

#include "GamePak.hpp"
#include "common/Types.hpp"

#include <vector>


namespace emu {

/* Maybe Later
struct Page {
    u8 *data; //If nullptr, it's not a fast page
    enum Permissions {
        READ_ONLY, WRITE_ONLY, READ_WRITE
    } permissions;
};
*/

enum AccessType {
    NON_SEQUENTIAL, SEQUENTIAL
};

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

    //For Debugger
    auto debugRead8(u32 address) -> u8;
    void debugWrite8(u32 address, u8 value);
    auto debugRead16(u32 address) -> u16;
    void debugWrite16(u32 address, u16 value);
    auto debugRead32(u32 address) -> u32;
    void debugWrite32(u32 address, u32 value);

    auto read8(u32 address, AccessType access) -> u8;
    void write8(u32 address, u8 value, AccessType access);
    auto read16(u32 address, AccessType access) -> u16; 
    void write16(u32 address, u16 value, AccessType access);
    auto read32(u32 address, AccessType access) -> u32;
    void write32(u32 address, u32 value, AccessType access);

    void loadROM(const std::vector<u8> &rom);
};

} //namespace emu