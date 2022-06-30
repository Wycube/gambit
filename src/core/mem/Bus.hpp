#pragma once

#include "core/cpu/Types.hpp"
#include "GamePak.hpp"
#include "core/Scheduler.hpp"
#include "core/Keypad.hpp"
#include "core/Timer.hpp"
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

    //For Debugger
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
        //Rest of general memory
    } m_mem;

    GamePak m_pak;
    Scheduler &m_scheduler;
    Keypad &m_keypad;
    Timer &m_timer;
    DMA &m_dma;
    PPU &m_ppu;


    auto readByte(u32 address) -> u8;
    void writeByte(u32 address, u8 value);
    auto readIO(u32 address) -> u8;
    void writeIO(u32 address, u8 value);
};

} //namespace emu