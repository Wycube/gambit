#include "Bus.hpp"


namespace emu {

Bus::Bus() {
    memset(&m_mem, 0, sizeof(m_mem));
}

auto Bus::read8(u32 address) -> u8 {
    address &= 0x0FFFFFFF;

    if(address <= 0x00003FFF) {
        return m_mem.bios[address];
    } else if(address <= 0x0203FFFF) {
        return m_mem.ewram[address - 0x02000000];
    } else if(address <= 0x03007FFF) {
        return m_mem.iwram[address - 0x03000000];
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
        m_mem.ewram[address - 0x02000000] = value;
    } else if(address <= 0x03007FFF) {
        m_mem.iwram[address - 0x03000000] = value;
    }
}

auto Bus::read16(u32 address) -> u16 {
    return read8(address) | (read8(address + 1) << 8);
}

void Bus::write16(u32 address, u16 value) {
    write8(address, value & 0xFF);
    write8(address + 1, (value >> 8) & 0xFF);
}

auto Bus::read32(u32 address) -> u32 {
    return read8(address) | (read8(address + 1) << 8) | (read8(address + 2) << 16) | (read8(address + 3) << 24);
}

void Bus::write32(u32 address, u32 value) {
    write8(address, value & 0xFF);
    write8(address + 1, (value >> 8) & 0xFF);
    write8(address + 2, (value >> 16) & 0xFF);
    write8(address + 3, (value >> 24) & 0xFF);
}

void Bus::loadROM(const std::vector<u8> &rom) {
    m_pak.loadROM(rom);
}

} //namespace emu