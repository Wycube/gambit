#pragma once

#include "GamePak.hpp"
#include "emulator/core/cpu/Types.hpp"
#include "common/Types.hpp"
#include <vector>
#include <atomic>


namespace emu {

class GBA;

class Bus final {
public:

    explicit Bus(GBA &core);

    void reset();

    void cycle(u32 cycles = 1);
    auto read8(u32 address) -> u8;
    auto read16(u32 address) -> u16; 
    auto readRotated16(u32 address) -> u32;
    auto read32(u32 address) -> u32;
    auto readRotated32(u32 address) -> u32;
    void write8(u32 address, u8 value);
    void write16(u32 address, u16 value);
    void write32(u32 address, u32 value);
    void requestInterrupt(InterruptSource source);

    auto getLoadedPak() -> GamePak&;
    void loadROM(std::vector<u8> &&rom);
    void loadBIOS(const std::vector<u8> &data);

    //Same as other read/writes but doesn't tick the scheduler
    auto debugRead8(u32 address) -> u8;
    auto debugRead16(u32 address) -> u16;
    auto debugRead32(u32 address) -> u32;
    void debugWrite8(u32 address, u8 value);
    void debugWrite16(u32 address, u16 value);
    void debugWrite32(u32 address, u32 value);

private:

    u8 bios[16_KiB];   //00000000 - 00003FFF
    u8 ewram[256_KiB]; //02000000 - 0203FFFF
    u8 iwram[32_KiB];  //03000000 - 03007FFF

    //Make access to IF atomic so requesting an interrupt
    //from another thread (InputDevice) is safe.
    std::atomic<u16> int_flags;
    u32 bios_open_bus;
    // u32 cpu_open_bus;
    u16 waitcnt;
    GBA &core;
    GamePak pak;

    template<typename T>
    auto read(u32 address) -> T;
    template<typename T>
    void write(u32 address, T value);

    auto readIO(u32 address) -> u8;
    void writeIO(u32 address, u8 value);
};

} //namespace emu