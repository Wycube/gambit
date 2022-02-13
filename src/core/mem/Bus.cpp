#include "Bus.hpp"


namespace emu {

Bus::Bus() {
    memset(&m_mem, 0, sizeof(m_mem));
}

auto Bus::debugRead8(u32 address) -> u8 {
    address &= 0x0FFFFFFF;
    u32 sub_address = address & 0xFFFFFF;

    switch(address >> 24) {
        case 0x0 : return m_mem.bios[sub_address]; //BIOS
        break;
        case 0x1 : //Not Used
        break;
        case 0x2 : return m_mem.ewram[sub_address]; //On-Board WRAM
        break;
        case 0x3 : return m_mem.iwram[sub_address]; //On-Chip WRAM
        break;
        case 0x4 : //I/O Registers
        break;
        case 0x5 : //BG/OBJ Palette RAM
        break;
        case 0x6 : //VRAM
        break;
        case 0x7 : //OAM - OBJ Attributes
        break;
        case 0x8 : 
        case 0x9 : return m_pak.read8(address - 0x8000000); //Pak ROM Waitstate 0
        break;
        case 0xA : 
        case 0xB : return m_pak.read8(address - 0xA000000); //Pak ROM Waitstate 1
        break;
        case 0xC : 
        case 0xD : return m_pak.read8(address - 0xC000000); //Pak ROM Waitstate 2
        break;
        case 0xE : //Pak SRAM
        break;
        case 0xF : //Not Used
        break;
    }

    return 0xFF;
}

auto Bus::debugRead16(u32 address) -> u16 {
    return debugRead8(address) | (debugRead8(address + 1) << 8);
}

auto Bus::debugRead32(u32 address) -> u32 {
    return debugRead8(address) | (debugRead8(address + 1) << 8) | (debugRead8(address + 2) << 16) | (debugRead8(address + 3) << 24);
}

void Bus::debugWrite8(u32 address, u8 value) {
    address &= 0x0FFFFFFF;
    u32 sub_address = address & 0xFFFFFF;

    switch(address >> 24) {
        case 0x0 : m_mem.bios[sub_address] = value; //BIOS
        break;
        case 0x1 : //Not Used
        break;
        case 0x2 : m_mem.ewram[sub_address] = value; //On-Board WRAM
        break;
        case 0x3 : m_mem.iwram[sub_address] = value; //On-Chip WRAM
        break;
        case 0x4 : //I/O Registers
        break;
        case 0x5 : //BG/OBJ Palette RAM
        break;
        case 0x6 : //VRAM
        break;
        case 0x7 : //OAM - OBJ Attributes
        break;
        case 0x8 : 
        case 0x9 : m_pak.write8(address - 0x8000000, value); //Pak ROM Waitstate 0
        break;
        case 0xA : 
        case 0xB : m_pak.write8(address - 0xA000000, value); //Pak ROM Waitstate 1
        break;
        case 0xC : 
        case 0xD : m_pak.write8(address - 0xC000000, value); //Pak ROM Waitstate 2
        break;
        case 0xE : //Pak SRAM
        break;
        case 0xF : //Not Used
        break;
    }
}

void Bus::debugWrite16(u32 address, u16 value) {
    debugWrite8(address, value & 0xFF);
    debugWrite8(address + 1, (value >> 8) & 0xFF);
}

void Bus::debugWrite32(u32 address, u32 value) {
    debugWrite8(address, value & 0xFF);
    debugWrite8(address + 1, (value >> 8) & 0xFF);
    debugWrite8(address + 2, (value >> 16) & 0xFF);
    debugWrite8(address + 3, (value >> 24) & 0xFF);
}

auto Bus::read8(u32 address, AccessType access) -> u8 {
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

auto Bus::read16(u32 address, AccessType access) -> u16 {
    return read8(address, access) | (read8(address + 1, access) << 8);
}

auto Bus::read32(u32 address, AccessType access) -> u32 {
    return read8(address, access) | (read8(address + 1, access) << 8) | (read8(address + 2, access) << 16) | (read8(address + 3, access) << 24);
}

void Bus::write8(u32 address, u8 value, AccessType access) {
    address &= 0x0FFFFFFF;

    if(address <= 0x00003FFF) {
        //BIOS is Read-Only
    } else if(address <= 0x0203FFFF) {
        m_mem.ewram[address - 0x02000000] = value;
    } else if(address <= 0x03007FFF) {
        m_mem.iwram[address - 0x03000000] = value;
    }
}

void Bus::write16(u32 address, u16 value, AccessType access) {
    write8(address, value & 0xFF, access);
    write8(address + 1, (value >> 8) & 0xFF, access);
}

void Bus::write32(u32 address, u32 value, AccessType access) {
    write8(address, value & 0xFF, access);
    write8(address + 1, (value >> 8) & 0xFF, access);
    write8(address + 2, (value >> 16) & 0xFF, access);
    write8(address + 3, (value >> 24) & 0xFF, access);
}

void Bus::loadROM(const std::vector<u8> &rom) {
    m_pak.loadROM(rom);
}

} //namespace emu