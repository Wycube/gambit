#include "Bus.hpp"
#include "emulator/core/GBA.hpp"
#include "common/Log.hpp"


namespace emu {

Bus::Bus(GBA &core) : pak(core.scheduler), core(core) {
    std::memset(bios, 0, sizeof(bios));
    reset();
}

void Bus::reset() {
    waitcnt = 0;
    pak.updateWaitstates(waitcnt);
    bios_open_bus = 0xE129F000;
    std::memset(ewram, 0, sizeof(ewram));
    std::memset(iwram, 0, sizeof(iwram));
}

auto Bus::read8(u32 address, AccessType access) -> u8 {
    core.scheduler.step(1);
    return read<u8>(address, access);
}

auto Bus::read16(u32 address, AccessType access) -> u16 {
    core.scheduler.step(1);
    return read<u16>(address, access);
}

auto Bus::readRotated16(u32 address, AccessType access) -> u32 {
    core.scheduler.step(1);
    return bits::ror(read<u16>(address, access), (address & 1) * 8);
}

auto Bus::read32(u32 address, AccessType access) -> u32 {
    core.scheduler.step(1);
    return read<u32>(address, access);
}

auto Bus::readRotated32(u32 address, AccessType access) -> u32 {
    core.scheduler.step(1);
    return bits::ror(read<u32>(address, access), (address & 3) * 8);
}

void Bus::write8(u32 address, u8 value, AccessType access) {
    core.scheduler.step(1);
    write<u8>(address, value, access);
}

void Bus::write16(u32 address, u16 value, AccessType access) {
    core.scheduler.step(1);
    write<u16>(address, value, access);
}

void Bus::write32(u32 address, u32 value, AccessType access) {
    core.scheduler.step(1);
    write<u32>(address, value, access);
}

void Bus::loadBIOS(const std::vector<u8> &data) {
    if(data.size() != sizeof(bios)) {
        LOG_FATAL("Failed to load BIOS: Invalid Size ({} bytes)!", data.size());
    }

    std::memcpy(bios, data.data(), sizeof(bios));

    //After startup, BIOS reads return the ARM instruction at 0xF4 (open bus).
    bios_open_bus = 0xE129F000;
}

// auto Bus::debugRead8(u32 address) -> u8 {
//     return read<u8>(address);
// }

// auto Bus::debugRead16(u32 address) -> u16 {
//     return read<u16>(address);
// }

// auto Bus::debugRead32(u32 address) -> u32 {
//     return read<u32>(address);
// }

// void Bus::debugWrite8(u32 address, u8 value) {
//     write<u8>(address, value);
// }

// void Bus::debugWrite16(u32 address, u16 value) {
//     write<u16>(address, value);
// }

// void Bus::debugWrite32(u32 address, u32 value) {
//     write<u32>(address, value);
// }

template<typename T>
auto Bus::read(u32 address, AccessType access) -> T {
    static_assert(std::is_integral_v<T>);
    static_assert(sizeof(T) <= 4);
    
    u32 sub_address = bits::align<T>(address) & 0xFFFFFF;
    u8 *memory_region = nullptr;
    u32 region_size = 0;
    T value = 0;

    switch(address >> 24) {
        case 0x0 : //BIOS
            if(address < 0x4000) {
                if(core.cpu.state.pc < 0x4000) {
                    memory_region = bios;
                    region_size = sizeof(bios);
                    bios_open_bus = bios[sub_address] | (bios[sub_address + 1] << 8) |
                        (bios[sub_address + 2] << 16) | (bios[sub_address + 3] << 24);
                } else {
                    // LOG_ERROR("BIOS Open Bus read");
                    return bios_open_bus;
                }
            }
            break;
        // case 0x1 : //Not Used
        break;
        case 0x2 : //On-Board WRAM
            core.scheduler.step(sizeof(T) == 4 ? 5 : 2);
            memory_region = ewram;
            region_size = sizeof(ewram);
            break;
        case 0x3 : //On-Chip WRAM
            memory_region = iwram;
            region_size = sizeof(iwram);
            break;
        case 0x4 : 
            for(size_t i = 0; i < sizeof(T); i++) {
                value |= (readIO(sub_address + i) << i * 8);
            }

            return value;
        case 0x5 : return core.ppu.readPalette<T>(sub_address); //Palette RAM
        case 0x6 : return core.ppu.readVRAM<T>(sub_address); //VRAM
        case 0x7 : return core.ppu.readOAM<T>(sub_address); //OAM - OBJ Attributes
        case 0x8 :
        case 0x9 :
        case 0xA :
        case 0xB :
        case 0xC :
        case 0xD :
        case 0xE :
        case 0xF : return pak.read<T>(address, access); //Cartridge
    }

    if(memory_region == nullptr) {
        // LOG_FATAL("Open Bus reads unimplemented, Address: 0x{:08X}", address);
        return 0;
    }

    for(size_t i = 0; i < sizeof(T); i++) {
        value |= (memory_region[(sub_address + i) % region_size] << i * 8);
    }

    return value;
}

template<typename T>
void Bus::write(u32 address, T value, AccessType access) {
    static_assert(std::is_integral_v<T>);
    static_assert(sizeof(T) <= 4);

    u32 sub_address = bits::align<T>(address) & 0xFFFFFF;
    u8 *memory_region = nullptr;
    u32 region_size = 0;

    switch(address >> 24) {
        case 0x0 : //BIOS (Read-only)
        break;
        case 0x1 : //Not Used
        break;
        case 0x2 : //On-Board WRAM
            core.scheduler.step(sizeof(T) == 4 ? 5 : 2);
            memory_region = ewram;
            region_size = sizeof(ewram);
        break;
        case 0x3 : //On-Chip WRAM
            memory_region = iwram;
            region_size = sizeof(iwram);
        break;
        case 0x4 : 
            for(size_t i = 0; i < sizeof(T); i++) {
                writeIO(sub_address + i, (value >> i * 8) & 0xFF);
            }
        break;
        case 0x5 : core.ppu.writePalette<T>(sub_address, value); //Palette RAM
        break;
        case 0x6 : core.ppu.writeVRAM<T>(sub_address, value); //VRAM
        break;
        case 0x7 : core.ppu.writeOAM<T>(sub_address, value); //OAM - OBJ Attributes
        break;
        case 0x8 :
        case 0x9 :
        case 0xA :
        case 0xB :
        case 0xC :
        case 0xD :
        case 0xE :
        case 0xF : pak.write<T>(address, value, access); //Cartridge
        break;
    }

    if(memory_region == nullptr) {
        return;
    }

    for(size_t i = 0; i < sizeof(T); i++) {
        memory_region[(sub_address + i) % region_size] = (value >> i * 8) & 0xFF;
    }
}

auto Bus::readIO(u32 address) -> u8 {
    if(address >= 0x400) {
        // LOG_FATAL("Open Bus IO reads unimplemented, Address: 0x04{:06X}", address);
        return 0;
    }

    if(address <= 0x56) {
        return core.ppu.readIO(address);
    }
    if(address >= 0x60 && address <= 0xA7) {
        return core.apu.read(address);
    }
    if(address >= 0xB0 && address < 0xE0) {
        return core.dma.read8(address);
    }
    if(address >= 0x100 && address <= 0x10F) {
        return core.timer.read8(address);
    }
    if(address >= 0x120 && address <= 0x12D) {
        return core.sio.read8(address);
    }
    if(address >= 0x130 && address <= 0x133) {
        return core.keypad.read8(address);
    }
    if(address >= 0x134 && address <= 0x15B) {
        return core.sio.read8(address);
    }

    //Post-boot flag
    if(address == 0x300) {
        return 0;
    }

    if(address >= 0x200 && address <= 0x20B) {
        return core.cpu.readIO(address);
    }

    // LOG_FATAL("Read from unimplemented IO at address: 0x04{:06X}", address);
    return 0;
}

void Bus::writeIO(u32 address, u8 value) {
    if(address >= 0x400) {
        return;
    }

    if(address <= 0x56) {
        core.ppu.writeIO(address, value);
        return;
    }
    if(address >= 0x60 && address <= 0xA7) {
        core.apu.write(address, value);
    }
    if(address >= 0xB0 && address < 0xE0) {
        core.dma.write8(address, value);
    }
    if(address >= 0x100 && address <= 0x10F) {
        return core.timer.write8(address, value);
    }
    if(address >= 0x120 && address <= 0x12D) {
        core.sio.write8(address, value);
        return;
    }
    if(address >= 0x130 && address <= 0x133) {
        core.keypad.write8(address, value);
        return;
    }
    if(address >= 0x134 && address <= 0x15B) {
        core.sio.write8(address, value);
        return;
    }

    //WAITCNT
    if(address == 0x204) {
        // LOG_INFO("Write to WAITCNT");
        waitcnt &= 0xFF00;
        waitcnt |= value;

        // static u8 cycles[] = {4, 3, 2, 8};

        // LOG_ERROR("SRAM waits: {}", cycles[waitcnt & 3]);
        pak.updateWaitstates(waitcnt);
    }
    if(address == 0x205) {
        // LOG_INFO("Write to WAITCNT");
        waitcnt &= 0xFF;
        waitcnt |= value << 8;
        waitcnt &= 0xFF7F; //Cart type flag
    
        // LOG_ERROR("Prefetch: {}", bits::get_bit<14>(waitcnt));
    }

    //HALTCNT
    if(address == 0x301) {
        if(value >> 7 == 0) {
            core.cpu.halt();
        }
    }

    if(address >= 0x200 && address <= 0x20B) {
        core.cpu.writeIO(address, value);
        return;
    }

    // LOG_FATAL("Write to unimplemented IO at address: 0x04{:06X}", address);
}

} //namespace emu