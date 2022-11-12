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

    Bus(GBA &core);

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
    void loadBIOS(const std::vector<u8> &bios);

    //Same as other read/writes but doesn't tick the scheduler
    auto debugRead8(u32 address) -> u8;
    auto debugRead16(u32 address) -> u16;
    auto debugRead32(u32 address) -> u32;
    void debugWrite8(u32 address, u8 value);
    void debugWrite16(u32 address, u16 value);
    void debugWrite32(u32 address, u32 value);

private:

    struct Memory {
        u8 bios[16_KiB];   //00000000 - 00003FFF
        u8 ewram[256_KiB]; //02000000 - 0203FFFF
        u8 iwram[32_KiB];  //03000000 - 03007FFF
        u8 io[1023];       //04000000 - 040003FE
    } m_mem;

    //Make access to IF atomic so possibly requesting an interrupt
    //from another thread (from InputDevice) is safe.
    std::atomic<u16> m_if;
    u32 m_bios_open_bus;
    // u32 m_cpu_open_bus;
    GamePak m_pak;
    GBA &m_core;

    template<typename T>
    auto read(u32 address) -> T;
    template<typename T>
    void write(u32 address, T value);

    auto readIO(u32 address) -> u8;
    void writeIO(u32 address, u8 value);
};

} //namespace emu