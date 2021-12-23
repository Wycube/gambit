#include "Bus.hpp"


namespace emu {

auto Bus::read8(u32 address) -> u8 {
    address &= 0x0FFFFFFF;

    if(address <= 0x00003FFF) {
        return m_mem.bios[address];
    } else if(address <= 0x0203FFFF) {
        return m_mem.wram1[address - 0x02000000];
    } else if(address <= 0x03007FFF) {
        return m_mem.wram2[address - 0x03000000];
    } else if(address >= 0x08000000) {
        return m_pak.read8(address - 0x08000000);
    }

    return 0xFF;
}

void Bus::write8(u32 address, u8 value) {
    address &= 0x0FFFFFFF;

    if(address <= 0x00003FFF) {
        //BIOS is Read-Only
    } else if(address <= 0x0203FFFF) {
        m_mem.wram1[address - 0x02000000] = value;
    } else if(address <= 0x03007FFF) {
        m_mem.wram2[address - 0x03000000] = value;
    }
}

auto Bus::read32(u32 address) -> u32 {
    u32 value = read8(address);
    value |= read8(address + 1) << 8;
    value |= read8(address + 2) << 16;
    value |= read8(address + 3) << 24;

    return value;
}

void Bus::loadROM(std::vector<u8> &rom) {
    m_pak.loadROM(rom);
}

} //namespace emu