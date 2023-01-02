#include "GamePak.hpp"
#include "save/EEPROM.hpp"
#include "save/Flash.hpp"
#include "save/SRAM.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

template auto GamePak::read<u8>(u32 address) -> u8;
template auto GamePak::read<u16>(u32 address) -> u16;
template auto GamePak::read<u32>(u32 address) -> u32;
template void GamePak::write<u8>(u32 address, u8 value);
template void GamePak::write<u16>(u32 address, u16 value);
template void GamePak::write<u32>(u32 address, u32 value);

template<typename T>
auto GamePak::read(u32 address) -> T {
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
            return 0;
    }

    if(aligned >= rom.size()) {
       return 0;
    }

    T value = 0;

    for(size_t i = 0; i < sizeof(T); i++) {
        value |= (rom[aligned + i] << i * 8);
    }

    return value;
}

template<typename T>
void GamePak::write(u32 address, T value) {
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
                if constexpr(sizeof(T) == 2) value >>= (address & 1) * 8;
                if constexpr(sizeof(T) == 4) value >>= (address & 3) * 8;

                return save->write(sub_address, value & 0xFF);
            }
            break;
    }
}

auto GamePak::getHeader() -> const GamePakHeader& {
    return header;
}

auto GamePak::getTitle() -> const std::string& {
    return title;
}

auto GamePak::getSave() -> std::shared_ptr<Save> {
    return save;
}

auto GamePak::size() -> u32 {
    return rom.size();
}

void GamePak::loadROM(std::vector<u8> &&rom) {
    this->rom = rom;
    parseHeader();

    //Get save type, only a string match so far
    if(!findSaveType()) {
        save = std::make_shared<None>();
        LOG_DEBUG("No save type detected!");
    }
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

auto GamePak::findSaveType() -> bool {
    for(size_t i = 0; i < rom.size(); i++) {
        char byte = static_cast<char>(rom[i]);

        if(byte == 'E' && (i + 5) < rom.size()) {
            //TODO: More stuff to detect size, possibly in EEPROM class
            const char next[6] = {static_cast<char>(rom[i + 1]),
                static_cast<char>(rom[i + 2]), static_cast<char>(rom[i + 3]), 
                static_cast<char>(rom[i + 4]), static_cast<char>(rom[i + 5]), '\0'};
            if(strcmp(next, "EPROM") == 0) {
                save = std::make_shared<EEPROM>(EEPROM_8K);
                LOG_DEBUG("EEPROM save type detected, assuming 8k");

                return true;
            }
        } else if(byte == 'S' && (i + 3) < rom.size()) {
            const char next[4] = {static_cast<char>(rom[i + 1]),
                static_cast<char>(rom[i + 2]), static_cast<char>(rom[i + 3]), '\0'};
            if(strcmp(next, "RAM") == 0) {
                save = std::make_shared<SRAM>();
                LOG_DEBUG("SRAM save type detected");

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
                        save = std::make_shared<Flash>(FLASH_64K);
                        LOG_DEBUG("Flash 64k save type detected");
                    
                        return true;
                    } else if(size == '1') {
                        save = std::make_shared<Flash>(FLASH_128K);
                        LOG_DEBUG("Flash 128k save type detected");

                        return true;
                    } else {
                        save = std::make_shared<Flash>(FLASH_64K);
                        LOG_DEBUG("Flash save type detected, assuming 64k");

                        return true;
                    }
                } else {
                    save = std::make_shared<Flash>(FLASH_64K);
                    LOG_DEBUG("Flash save type detected, assuming 64k");

                    return true;
                }
            }
        }
    }

    return false;
}

} //namespace emu