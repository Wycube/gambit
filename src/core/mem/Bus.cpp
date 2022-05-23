#include "Bus.hpp"

#include "common/Log.hpp"


namespace emu {

Bus::Bus(Scheduler &scheduler, PPU &ppu) : m_scheduler(scheduler), m_ppu(ppu) {
    reset();
}

void Bus::wait(u32 cycles) {
    m_scheduler.step(cycles);
}

auto Bus::read_byte(u32 address) -> u8 {
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
        case 0x4 : return read_io(address - 0x4000000);
        break;
        case 0x5 :
        case 0x6 :
        case 0x7 : return m_ppu.read8(address); //BG/OBJ Palette RAM / VRAM / OAM - OBJ Attributes
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

void Bus::write_byte(u32 address, u8 value) {
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
        case 0x4 : write_io(address - 0x4000000, value);
        break;
        case 0x5 :
        case 0x6 :
        case 0x7 : m_ppu.write8(address, value); //BG/OBJ Palette RAM / VRAM / OAM - OBJ Attributes
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

auto Bus::read_io(u32 address) -> u8 {
    if((address & ~1) == 0) {
        return m_ppu.read8(0x04000000 + address);
    }

    return m_mem.io[address];
}

void Bus::write_io(u32 address, u8 value) {
    if((address & ~1) == 0) {
        m_ppu.write8(0x04000000 + address, value);
        
        return;
    }

    m_mem.io[address] = value;
}

void Bus::reset() {
    memset(&m_mem, 0, sizeof(m_mem));
}

auto Bus::read8(u32 address) -> u8 {
    cycle();
    LOG_DEBUG("8-bit read at 0x{:08X}", address);

    return read_byte(address);
}

auto Bus::read16(u32 address) -> u16 {
    cycle();
    LOG_DEBUG("16-bit read at 0x{:08X}", address);

    return read_byte(address) | (read_byte(address + 1) << 8);
}

auto Bus::read32(u32 address) -> u32 {
    cycle();
    LOG_DEBUG("32-bit read at 0x{:08X}", address);
    
    return read_byte(address) | (read_byte(address + 1) << 8) | (read_byte(address + 2) << 16) | (read_byte(address + 3) << 24);
}

void Bus::write8(u32 address, u8 value) {
    cycle();
    LOG_DEBUG("8-bit write of 0x{:02X} to 0x{:08X}", value, address);
    
    write_byte(address, value);
}

void Bus::write16(u32 address, u16 value) {
    cycle();
    LOG_DEBUG("16-bit write of 0x{:02X} to 0x{:08X}", value, address);
    
    write_byte(address, value & 0xFF);
    write_byte(address + 1, (value >> 8) & 0xFF);
}

void Bus::write32(u32 address, u32 value) {
    cycle();
    LOG_DEBUG("32-bit write of 0x{:02X} to 0x{:08X}", value, address);

    write_byte(address, value & 0xFF);
    write_byte(address + 1, (value >> 8) & 0xFF);
    write_byte(address + 2, (value >> 16) & 0xFF);
    write_byte(address + 3, (value >> 24) & 0xFF);
}

void Bus::cycle() {
    m_scheduler.step(1);
}

auto Bus::getLoadedPak() -> GamePak& {
    return m_pak;
}

void Bus::loadROM(const std::vector<u8> &rom) {
    m_pak.loadROM(rom);
}

void Bus::loadBIOS(const std::vector<u8> &bios) {
    if(bios.size() > sizeof(m_mem.bios)) {
        LOG_FATAL("Failed to load bios: Too Large ({} bytes)!", bios.size());
    }

    memcpy(m_mem.bios, bios.data(), sizeof(m_mem.bios));
}

auto Bus::debugRead8(u32 address) -> u8 {
    return read_byte(address);
}

auto Bus::debugRead16(u32 address) -> u16 {
    return debugRead8(address) | (debugRead8(address + 1) << 8);
}

auto Bus::debugRead32(u32 address) -> u32 {
    return debugRead8(address) | (debugRead8(address + 1) << 8) | (debugRead8(address + 2) << 16) | (debugRead8(address + 3) << 24);
}

void Bus::debugWrite8(u32 address, u8 value) {
    write_byte(address, value);
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

} //namespace emu