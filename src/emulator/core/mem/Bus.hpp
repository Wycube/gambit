#pragma once

#include "GamePak.hpp"
#include "EEPROM.hpp"
#include "emulator/core/cpu/Types.hpp"
#include "emulator/core/Scheduler.hpp"
#include "emulator/core/Keypad.hpp"
#include "emulator/core/Timer.hpp"
#include "common/Types.hpp"
#include <vector>


namespace emu {

class DMA;
class PPU;

class Bus {
public:

    Bus(Scheduler &scheduler, Keypad &keypad, Timer &timer, DMA &dma, PPU &ppu);

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

    GamePak m_pak;
    EEPROM m_eeprom;
    Scheduler &m_scheduler;
    Keypad &m_keypad;
    Timer &m_timer;
    DMA &m_dma;
    PPU &m_ppu;

    template<typename T>
    auto read(u32 address) -> T;
    template<typename T>
    void write(u32 address, T value);

    auto readIO(u32 address) -> u8;
    void writeIO(u32 address, u8 value);
};

} //namespace emu