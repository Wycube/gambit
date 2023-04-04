#include "GamePak.hpp"
#include "emulator/core/GBA.hpp"
#include "save/EEPROM.hpp"
#include "save/Flash.hpp"
#include "save/SRAM.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"
#include "common/File.hpp"

constexpr u32 SRAM_WAIT_CYCLES[4] = {4, 3, 2, 8};
constexpr u32 WSN_WAIT_CYCLES[4] = {4, 3, 2, 8};
constexpr u32 WS0S_WAIT_CYCLES[2] = {2, 1};
constexpr u32 WS1S_WAIT_CYCLES[2] = {4, 1};
constexpr u32 WS2S_WAIT_CYCLES[2] = {8, 1};


namespace emu {

template auto GamePak::read<u8>(u32 address, AccessType access) -> u8;
template auto GamePak::read<u16>(u32 address, AccessType access) -> u16;
template auto GamePak::read<u32>(u32 address, AccessType access) -> u32;
template void GamePak::write<u8>(u32 address, u8 value, AccessType access);
template void GamePak::write<u16>(u32 address, u16 value, AccessType access);
template void GamePak::write<u32>(u32 address, u32 value, AccessType access);

GamePak::GamePak(Scheduler &scheduler) : scheduler(scheduler) { }

template<typename T>
auto GamePak::read(u32 address, AccessType access) -> T {
    u32 sub_address = address & 0xFFFFFF;
    u32 aligned = bits::align<T>(address) & 0x1FFFFFF;

    switch(address >> 24) {
        case 0xD : //EEPROM
            if(save->getType() == EEPROM_512 || save->getType() == EEPROM_8K) {
                return save->read(sub_address);
            }
            return 0;
        case 0xE :
        case 0xF : //SRAM or Flash
            if(save->getType() == SRAM_32K || save->getType() == FLASH_64K || save->getType() == FLASH_128K) {
                return save->read(sub_address) * 0x01010101;
            }
            return 0xFF;
    }

    if(aligned >= rom.size()) {
       return 0;
    }

    switch(address >> 24) {
        case 0x8 :
        case 0x9 :
            if(sizeof(T) == 4) {
                scheduler.step(ws0_s + (access == SEQUENTIAL ? ws0_s : ws0_n));
            } else {
                scheduler.step(access == SEQUENTIAL ? ws0_s : ws0_n);
            }
            break;

        case 0xA :
        case 0xB :
            if(sizeof(T) == 4) {
                scheduler.step(ws1_s + (access == SEQUENTIAL ? ws1_s : ws1_n));
            } else {
                scheduler.step(access == SEQUENTIAL ? ws1_s : ws1_n);
            }
            break;
        
        case 0xC :
        case 0xD :
            if(sizeof(T) == 4) {
                scheduler.step(ws2_s + (access == SEQUENTIAL ? ws2_s : ws2_n));
            } else {
                scheduler.step(access == SEQUENTIAL ? ws2_s : ws2_n);
            }
            break;
    }

    T value = 0;
    for(size_t i = 0; i < sizeof(T); i++) {
        value |= (rom[aligned + i] << i * 8);
    }

    return value;
}

template<typename T>
void GamePak::write(u32 address, T value, AccessType access) {
    u32 sub_address = address & 0xFFFFFF;

    switch(address >> 24) {
        case 0xD : //EEPROM
            if(sizeof(T) == 2 && (save->getType() == EEPROM_512 || save->getType() == EEPROM_8K)) {
                return save->write(sub_address, value & 0xFF);
            }
            break;
        case 0xE :
        case 0xF : //SRAM or Flash
            if(save->getType() == SRAM_32K || save->getType() == FLASH_64K || save->getType() == FLASH_128K) {
                u32 mask = save->getType() == SRAM_32K ? 0x7FFF : save->getType() == FLASH_64K ? 0xFFFF : 0x1FFFF;
                if constexpr(sizeof(T) == 2) value >>= (address & 1) * 8;
                if constexpr(sizeof(T) == 4) value >>= (address & 3) * 8;

                return save->write(sub_address & mask, value & 0xFF);
            }
            break;
    }
}

void GamePak::updateWaitstates(u16 waitcnt) {
    sram_waitstate = SRAM_WAIT_CYCLES[waitcnt & 3];
    ws0_n = WSN_WAIT_CYCLES[(waitcnt >> 2) & 3];
    ws0_s = WS0S_WAIT_CYCLES[(waitcnt >> 4) & 1];
    ws1_n = WSN_WAIT_CYCLES[(waitcnt >> 5) & 3];
    ws1_s = WS1S_WAIT_CYCLES[(waitcnt >> 7) & 1];
    ws2_n = WSN_WAIT_CYCLES[(waitcnt >> 8) & 3];
    ws2_s = WS2S_WAIT_CYCLES[(waitcnt >> 10) & 1];
    // LOG_INFO("Updated waitstate sram: {}, WS0 1st: {}, WS0 2nd: {}", sram_waitstate, ws0_n, ws0_s);
}

auto GamePak::getHeader() -> const GamePakHeader& {
    return header;
}

auto GamePak::getTitle() -> const std::string& {
    return title;
}

auto GamePak::size() -> u32 {
    return rom.size();
}

auto GamePak::loadFile(const std::string &path) -> bool {
    std::vector<u8> file_data = common::loadFileBytes(path.c_str());

    if(file_data.empty()) {
        return false;
    }

    rom = std::move(file_data);
    parseHeader();

    //Get save type, only a string match so far
    if(!findSaveType(path.substr(0, path.find_last_of(".")) + ".sav")) {
        save = std::make_unique<None>();
        LOG_DEBUG("No save type detected!");
    }

    return true;
}

void GamePak::unload() {
    //This is the equivilent of pulling the game cartridge out, essentially
    rom.clear();
    save.reset();
}

void GamePak::parseHeader() {
    header.entry_point = (rom[3] << 24) | (rom[2] << 16) | (rom[1] << 8) | rom[0];
    std::memcpy(header.logo, &rom[4], sizeof(header.logo));
    std::memcpy(header.title, &rom[0xA0], sizeof(header.title));
    std::memcpy(header.game_code, &rom[0xAC], sizeof(header.game_code));
    std::memcpy(header.maker_code, &rom[0xB0], sizeof(header.maker_code));
    header.fixed = rom[0xB2]; //Must be 0x96
    header.unit_code = rom[0xB3];
    header.device_type = rom[0xB4];
    std::memcpy(header.reserved_0, &rom[0xB5], sizeof(header.reserved_0)); //Should be zero filled
    header.version = rom[0xBC];
    header.checksum = rom[0xBD];
    std::memcpy(header.reserved_1, &rom[0xBE], sizeof(header.reserved_1)); //Also should be zero filled

    char title_str[13];
    memcpy(title_str, header.title, sizeof(header.title));
    title_str[12] = '\0';
    title = title_str;
}

auto GamePak::findSaveType(const std::string &path) -> bool {
    for(size_t i = 0; i < rom.size(); i++) {
        char byte = static_cast<char>(rom[i]);

        if(byte == 'E' && (i + 5) < rom.size()) {
            //TODO: More stuff to detect size, possibly in EEPROM class
            const char next[6] = {static_cast<char>(rom[i + 1]),
                static_cast<char>(rom[i + 2]), static_cast<char>(rom[i + 3]), 
                static_cast<char>(rom[i + 4]), static_cast<char>(rom[i + 5]), '\0'};
            if(strcmp(next, "EPROM") == 0) {
                LOG_DEBUG("EEPROM save type detected, assuming 8k");
                save = std::make_unique<EEPROM>(EEPROM_8K, path);

                return true;
            }
        } else if(byte == 'S' && (i + 3) < rom.size()) {
            const char next[4] = {static_cast<char>(rom[i + 1]),
                static_cast<char>(rom[i + 2]), static_cast<char>(rom[i + 3]), '\0'};
            if(strcmp(next, "RAM") == 0) {
                LOG_DEBUG("SRAM save type detected");
                save = std::make_unique<SRAM>(path);

                return true;
            }
        } else if(byte == 'F' && (i + 4) < rom.size()) {
            const char next[5] = {static_cast<char>(rom[i + 1]),
                static_cast<char>(rom[i + 2]), static_cast<char>(rom[i + 3]), 
                static_cast<char>(rom[i + 4]), '\0'};
            if(strcmp(next, "LASH") == 0) {
                if((i + 5) < rom.size()) {
                    char size = static_cast<char>(rom[i + 5]);

                    if(size == '5') {
                        LOG_DEBUG("Flash 64k save type detected");
                        save = std::make_unique<Flash>(FLASH_64K, path);
                    
                        return true;
                    } else if(size == '1') {
                        LOG_DEBUG("Flash 128k save type detected");
                        save = std::make_unique<Flash>(FLASH_128K, path);

                        return true;
                    } else {
                        save = std::make_unique<Flash>(FLASH_64K, path);
                        LOG_DEBUG("Flash save type detected, assuming 64k");

                        return true;
                    }
                } else {
                    LOG_DEBUG("Flash save type detected, assuming 64k");
                    save = std::make_unique<Flash>(FLASH_64K, path);

                    return true;
                }
            }
        }
    }

    return false;
}

} //namespace emu