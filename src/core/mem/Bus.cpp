#include "Bus.hpp"
#include "core/ppu/PPU.hpp"
#include "common/Log.hpp"


namespace emu {

Bus::Bus(Scheduler &scheduler, Keypad &keypad, PPU &ppu) : m_scheduler(scheduler), m_keypad(keypad), m_ppu(ppu) {
    reset();
}

void Bus::reset() {
    memset(&m_mem, 0, sizeof(m_mem));
}

void Bus::cycle(u32 cycles) {
    m_scheduler.step(cycles);
}

auto Bus::read8(u32 address) -> u8 {
    cycle();
    //LOG_DEBUG("8-bit read at 0x{:08X}", address);

    return readByte(address);
}

auto Bus::read16(u32 address) -> u16 {
    cycle();
    //LOG_DEBUG("16-bit read at 0x{:08X}", address);

    return readByte(address) | (readByte(address + 1) << 8);
}

auto Bus::read32(u32 address) -> u32 {
    cycle();
    //LOG_DEBUG("32-bit read at 0x{:08X}", address);
    
    return readByte(address) | (readByte(address + 1) << 8) | (readByte(address + 2) << 16) | (readByte(address + 3) << 24);
}

void Bus::write8(u32 address, u8 value) {
    cycle();
    //LOG_DEBUG("8-bit write of 0x{:02X} to 0x{:08X}", value, address);
    
    writeByte(address, value);
}

void Bus::write16(u32 address, u16 value) {
    cycle();
    //LOG_DEBUG("16-bit write of 0x{:02X} to 0x{:08X}", value, address);
    
    writeByte(address, value & 0xFF);
    writeByte(address + 1, (value >> 8) & 0xFF);
}

void Bus::write32(u32 address, u32 value) {
    cycle();
    //LOG_DEBUG("32-bit write of 0x{:02X} to 0x{:08X}", value, address);

    writeByte(address, value & 0xFF);
    writeByte(address + 1, (value >> 8) & 0xFF);
    writeByte(address + 2, (value >> 16) & 0xFF);
    writeByte(address + 3, (value >> 24) & 0xFF);
}

void Bus::requestInterrupt(InterruptSource source) {
    //Set the flag in IF at address 0x04000202
    m_mem.io[2] |= source;
    m_mem.io[3] |= source >> 8;
}

auto Bus::getLoadedPak() -> GamePak& {
    return m_pak;
}

void Bus::loadROM(std::vector<u8> &&rom) {
    m_pak.loadROM(std::move(rom));
}

void Bus::loadBIOS(const std::vector<u8> &bios) {
    if(bios.size() > sizeof(m_mem.bios)) {
        LOG_FATAL("Failed to load bios: Too Large ({} bytes)!", bios.size());
    }

    memcpy(m_mem.bios, bios.data(), sizeof(m_mem.bios));
}

auto Bus::debugRead8(u32 address) -> u8 {
    return readByte(address);
}

auto Bus::debugRead16(u32 address) -> u16 {
    return debugRead8(address) | (debugRead8(address + 1) << 8);
}

auto Bus::debugRead32(u32 address) -> u32 {
    return debugRead8(address) | (debugRead8(address + 1) << 8) | (debugRead8(address + 2) << 16) | (debugRead8(address + 3) << 24);
}

void Bus::debugWrite8(u32 address, u8 value) {
    writeByte(address, value);
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

auto Bus::readByte(u32 address) -> u8 {
    u32 sub_address = address & 0xFFFFFF;

    switch((address >> 24) & 0xF) {
        case 0x0 : return m_mem.bios[sub_address]; //BIOS
        break;
        case 0x1 : //Not Used
        break;
        case 0x2 : return m_mem.ewram[sub_address % sizeof(m_mem.ewram)]; //On-Board WRAM
        break;
        case 0x3 : return m_mem.iwram[sub_address % sizeof(m_mem.iwram)]; //On-Chip WRAM
        break;
        case 0x4 : return readIO(sub_address);
        break;
        case 0x5 : return m_ppu.readPalette(sub_address); //BG/OBJ Palette RAM
        break;
        case 0x6 : return m_ppu.readVRAM(sub_address); //VRAM
        break;
        case 0x7 : return m_ppu.readOAM(sub_address); //OAM - OBJ Attributes
        break;
        case 0x8 : 
        case 0x9 : //Pak ROM Waitstate 0
        case 0xA : 
        case 0xB : //Pak ROM Waitstate 1
        case 0xC : 
        case 0xD : return m_pak.read8(sub_address); //Pak ROM Waitstate 2
        break;
        case 0xE : //Pak SRAM
        break;
        case 0xF : //Not Used
        break;
    }

    return 0xFF;
}

void Bus::writeByte(u32 address, u8 value) {
    u32 sub_address = address & 0xFFFFFF;

    switch((address >> 24) & 0xF) {
        case 0x0 : m_mem.bios[sub_address] = value; //BIOS
        break;
        case 0x1 : //Not Used
        break;
        case 0x2 : m_mem.ewram[sub_address % sizeof(m_mem.ewram)] = value; //On-Board WRAM
        break;
        case 0x3 : m_mem.iwram[sub_address % sizeof(m_mem.iwram)] = value; //On-Chip WRAM
        break;
        case 0x4 : writeIO(sub_address, value);
        break;
        case 0x5 : m_ppu.writePalette(sub_address, value); //BG/OBJ Palette RAM
        break;
        case 0x6 : m_ppu.writeVRAM(sub_address, value); //VRAM
        break;
        case 0x7 : m_ppu.writeOAM(sub_address, value); //OAM - OBJ Attributes
        break;
        case 0x8 : 
        case 0x9 : //Pak ROM Waitstate 0
        case 0xA : 
        case 0xB : //Pak ROM Waitstate 1
        case 0xC : 
        case 0xD : m_pak.write8(sub_address, value); //Pak ROM Waitstate 2
        break;
        case 0xE : //Pak SRAM
        break;
        case 0xF : //Not Used
        break;
    }
}

auto Bus::readIO(u32 address) -> u8 {
    if(address <= 0x56) {
        return m_ppu.readIO(address);
    }
    if(address >= 0x130 && address <= 0x133) {
        return m_keypad.read8(address);
    }

    return m_mem.io[address];
}

void Bus::writeIO(u32 address, u8 value) {
    if(address <= 0x56) {
        m_ppu.writeIO(address, value);
        return;
    }
    if(address >= 0x130 && address <= 0x133) {
        m_keypad.write8(address, value);
        return;
    }

    m_mem.io[address] = value;
}

} //namespace emu