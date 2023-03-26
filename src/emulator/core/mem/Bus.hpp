#pragma once

#include "Types.hpp"
#include "GamePak.hpp"
#include "emulator/core/cpu/Types.hpp"
#include "common/Types.hpp"


namespace emu {

class GBA;

class Bus final {
public:

    explicit Bus(GBA &core);

    void reset();

    auto read8(u32 address, AccessType access) -> u8;
    auto read16(u32 address, AccessType access) -> u16; 
    auto readRotated16(u32 address, AccessType access) -> u32;
    auto read32(u32 address, AccessType access) -> u32;
    auto readRotated32(u32 address, AccessType access) -> u32;
    void write8(u32 address, u8 value, AccessType access);
    void write16(u32 address, u16 value, AccessType access);
    void write32(u32 address, u32 value, AccessType access);

    void loadBIOS(const std::vector<u8> &data);

    //Same as other read/writes but doesn't tick the scheduler
    // auto debugRead8(u32 address) -> u8;
    // auto debugRead16(u32 address) -> u16;
    // auto debugRead32(u32 address) -> u32;
    // void debugWrite8(u32 address, u8 value);
    // void debugWrite16(u32 address, u16 value);
    // void debugWrite32(u32 address, u32 value);
    GamePak pak;

private:

    template<typename T>
    auto read(u32 address, AccessType access) -> T;
    template<typename T>
    void write(u32 address, T value, AccessType access);

    auto readIO(u32 address) -> u8;
    void writeIO(u32 address, u8 value);

    u8 bios[16_KiB];   //00000000 - 00003FFF
    u8 ewram[256_KiB]; //02000000 - 0203FFFF
    u8 iwram[32_KiB];  //03000000 - 03007FFF

    u32 bios_open_bus;
    // u32 cpu_open_bus;
    u16 waitcnt;
    GBA &core;
};

} //namespace emu