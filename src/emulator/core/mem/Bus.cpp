#include "Bus.hpp"
#include "emulator/core/GBA.hpp"
#include "common/Log.hpp"


namespace emu {

Bus::Bus(GBA &core) : m_core(core) {
    reset();
}

void Bus::reset() {
    memset(&m_mem.ewram, 0, sizeof(m_mem.ewram));
    memset(&m_mem.iwram, 0, sizeof(m_mem.iwram));
    memset(&m_mem.io, 0, sizeof(m_mem.io));
}

void Bus::cycle(u32 cycles) {
    m_core.scheduler.step(cycles);
}

auto Bus::read8(u32 address) -> u8 {
    cycle();
    return read<u8>(address);
}

auto Bus::read16(u32 address) -> u16 {
    cycle();
    return read<u16>(bits::align<u16>(address));
}

auto Bus::readRotated16(u32 address) -> u32 {
    cycle();
    return bits::ror(read<u16>(bits::align<u16>(address)), (address & 1) * 8);
}

auto Bus::read32(u32 address) -> u32 {
    cycle();
    return read<u32>(bits::align<u32>(address));
}

auto Bus::readRotated32(u32 address) -> u32 {
    cycle();
    return bits::ror(read<u32>(bits::align<u32>(address)), (address & 3) * 8);
}

void Bus::write8(u32 address, u8 value) {
    cycle();
    write<u8>(address, value);
}

void Bus::write16(u32 address, u16 value) {
    cycle();
    write<u16>(bits::align<u16>(address), value);
}

void Bus::write32(u32 address, u32 value) {
    cycle();
    write<u32>(bits::align<u32>(address), value);
}

void Bus::requestInterrupt(InterruptSource source) {
    //Set the flag in IF at address 0x04000202
    m_mem.io[0x202] |= source;
    m_mem.io[0x203] |= source >> 8;
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
    return read<u8>(address);
}

auto Bus::debugRead16(u32 address) -> u16 {
    return read<u16>(address);
}

auto Bus::debugRead32(u32 address) -> u32 {
    return read<u32>(address);
}

void Bus::debugWrite8(u32 address, u8 value) {
    write<u8>(address, value);
}

void Bus::debugWrite16(u32 address, u16 value) {
    write<u16>(address, value);
}

void Bus::debugWrite32(u32 address, u32 value) {
    write<u32>(address, value);
}

template<typename T>
auto Bus::read(u32 address) -> T {
    static_assert(std::is_integral_v<T>);
    static_assert(sizeof(T) <= 4);
    
    u32 sub_address = address & 0xFFFFFF;
    u8 *memory_region = nullptr;
    u32 region_size = 0;
    T value = 0;

    switch((address >> 24) & 0xF) {
        case 0x0 : //BIOS
            memory_region = m_mem.bios;
            region_size = sizeof(m_mem.bios);
        break;
        case 0x1 : //Not Used
        break;
        case 0x2 : //On-Board WRAM
            memory_region = m_mem.ewram;
            region_size = sizeof(m_mem.ewram);
        break;
        case 0x3 : //On-Chip WRAM
            memory_region = m_mem.iwram;
            region_size = sizeof(m_mem.iwram);
        break;
        case 0x4 : 
            for(int i = 0; i < sizeof(T); i++) {
                value |= (readIO((sub_address + i) % sizeof(m_mem.io)) << i * 8);
            }

            return value;
        break;
        case 0x5 : return m_core.ppu.readPalette<T>(sub_address); //Palette RAM
        break;
        case 0x6 : return m_core.ppu.readVRAM<T>(sub_address); //VRAM
        break;
        case 0x7 : return m_core.ppu.readOAM<T>(sub_address); //OAM - OBJ Attributes
        break;
        case 0x8 : 
        case 0x9 : //Pak ROM Waitstate 0
        case 0xA : 
        case 0xB : //Pak ROM Waitstate 1
        case 0xC : //return m_pak.read<T>(sub_address); //Pak ROM Waitstate 2
        case 0xD : //if constexpr (sizeof(T) == 2) return m_eeprom.read();
        case 0xE : return m_pak.read<T>(address); //Pak SRAM
        break;
        case 0xF : //Not Used
        break;
    }

    if(memory_region == nullptr) {
        return 0;
    }

    for(int i = 0; i < sizeof(T); i++) {
        value |= (memory_region[(sub_address + i) % region_size] << i * 8);
    }

    return value;
}

template<typename T>
void Bus::write(u32 address, T value) {
    static_assert(std::is_integral_v<T>);
    static_assert(sizeof(T) <= 4);
    
    u32 sub_address = address & 0xFFFFFF;
    u8 *memory_region = nullptr;
    u32 region_size = 0;

    switch((address >> 24) & 0xF) {
        case 0x0 : //BIOS (Read-only)
        break;
        case 0x1 : //Not Used
        break;
        case 0x2 : //On-Board WRAM
            memory_region = m_mem.ewram;
            region_size = sizeof(m_mem.ewram);
        break;
        case 0x3 : //On-Chip WRAM
            memory_region = m_mem.iwram;
            region_size = sizeof(m_mem.iwram);
        break;
        case 0x4 : 
            for(int i = 0; i < sizeof(T); i++) {
                writeIO((sub_address + i) % sizeof(m_mem.io), (value >> i * 8) & 0xFF);
            }
        break;
        case 0x5 : m_core.ppu.writePalette<T>(sub_address, value); //Palette RAM
        break;
        case 0x6 : m_core.ppu.writeVRAM<T>(sub_address, value); //VRAM
        break;
        case 0x7 : m_core.ppu.writeOAM<T>(sub_address, value); //OAM - OBJ Attributes
        break;
        case 0x8 : 
        case 0x9 : //Pak ROM Waitstate 0
        case 0xA : 
        case 0xB : //Pak ROM Waitstate 1
        case 0xC : //m_pak.write<T>(sub_address, value); //Pak ROM Waitstate 2
        case 0xD : //if constexpr (sizeof(T) == 2) m_eeprom.write(value);//LOG_DEBUG("Writing {:08X} to 0x0DXXXXXX address", value); return;
        case 0xE : m_pak.write<T>(address, value); //SRAM
            //LOG_FATAL("Write to SRAM");
        break;
        case 0xF : //Not Used
        break;
    }

    if(memory_region == nullptr) {
        return;
    }

    for(int i = 0; i < sizeof(T); i++) {
        memory_region[(sub_address + i) % region_size] = (value >> i * 8) & 0xFF;
    }
}

auto Bus::readIO(u32 address) -> u8 {
    if(address <= 0x56) {
        return m_core.ppu.readIO(address);
    }

    //BIOS SOUNDBIAS Stub
    if(address == 0x89) {
        return 2;
    }

    if(address >= 0xB0 && address <= 0xE0) {
        return m_core.dma.read8(address);
    }
    if(address >= 0x100 && address <= 0x10F) {
        return m_core.timer.read8(address);
    }
    if(address >= 0x130 && address <= 0x133) {
        return m_core.keypad.read8(address);
    }

    return m_mem.io[address];
}

void Bus::writeIO(u32 address, u8 value) {
    if(address <= 0x56) {
        m_core.ppu.writeIO(address, value);
        return;
    }
    if(address >= 0xB0 && address <= 0xE0) {
        m_core.dma.write8(address, value);
    }
    if(address >= 0x100 && address <= 0x10F) {
        return m_core.timer.write8(address, value);
    }
    if(address >= 0x130 && address <= 0x133) {
        m_core.keypad.write8(address, value);
        return;
    }
    if((address & ~3) == 0x208) {
        if(address == 0x208) {
            m_mem.io[address] = value & 1;
        }
        return;
    }
    if((address & ~1) == 0x202) {
        m_mem.io[address] &= ~value;
        return;
    }

    //HALTCNT
    if(address == 0x301) {
        if(value >> 7 == 0) {
            m_core.cpu.halt();
        }
    }

    m_mem.io[address] = value;
}

} //namespace emu