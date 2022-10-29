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
            if(m_save->getType() == EEPROM_512 || m_save->getType() == EEPROM_8K) {
                return m_save->read(sub_address);
            }
            return 0;
        case 0xE :
        case 0xF : //SRAM or Flash
            if(m_save->getType() == SRAM_32K || m_save->getType() == FLASH_64K || m_save->getType() == FLASH_128K) {
                return m_save->read(sub_address) * 0x01010101;
            }
            return 0;
    }

    if(aligned >= m_rom.size()) {
       return 0;
    }

    T value = 0;

    for(size_t i = 0; i < sizeof(T); i++) {
        value |= (m_rom[aligned + i] << i * 8);
    }

    return value;
}

template<typename T>
void GamePak::write(u32 address, T value) {
    u32 sub_address = address & 0xFFFFFF;

    switch(address >> 24) {
        case 0xD : //EEPROM
            if(sizeof(T) == 2 && (m_save->getType() == EEPROM_512 || m_save->getType() == EEPROM_8K)) {
                return m_save->write(sub_address, value & 0xFF);
            }
            break;
        case 0xE :
        case 0xF : //SRAM or Flash
            if(m_save->getType() == SRAM_32K || m_save->getType() == FLASH_64K || m_save->getType() == FLASH_128K) {
                if constexpr(sizeof(T) == 2) value >>= (address & 1) * 8;
                if constexpr(sizeof(T) == 4) value >>= (address & 3) * 8;

                return m_save->write(sub_address, value & 0xFF);
            }
            break;
    }
}

auto GamePak::getHeader() -> const GamePakHeader& {
    return m_header;
}

auto GamePak::getTitle() -> const std::string& {
    return m_title;
}

auto GamePak::getSave() -> std::shared_ptr<Save> {
    return m_save;
}

auto GamePak::size() -> u32 {
    return m_rom.size();
}

void GamePak::loadROM(std::vector<u8> &&rom) {
    m_rom = rom;
    parseHeader();

    //Get save type, somehow
    if(!findSaveType()) {
        m_save = std::make_shared<None>();
        LOG_INFO("No save type detected!");
    }
}

void GamePak::parseHeader() {
    m_header.entry_point = (m_rom[3] << 24) | (m_rom[2] << 16) | (m_rom[1] << 8) | m_rom[0];
    std::memcpy(m_header.logo, &m_rom[4], sizeof(m_header.logo));
    std::memcpy(m_header.title, &m_rom[0xA0], sizeof(m_header.title));
    std::memcpy(m_header.game_code, &m_rom[0xAC], sizeof(m_header.game_code));
    std::memcpy(m_header.maker_code, &m_rom[0xB0], sizeof(m_header.maker_code));
    m_header.fixed = m_rom[0xB2]; //Must be 0x96
    m_header.unit_code = m_rom[0xB3];
    m_header.device_type = m_rom[0xB4];
    std::memcpy(m_header.reserved_0, &m_rom[0xB5], sizeof(m_header.reserved_0)); //Should be zero filled
    m_header.version = m_rom[0xBC];
    m_header.checksum = m_rom[0xBD];
    std::memcpy(m_header.reserved_1, &m_rom[0xBE], sizeof(m_header.reserved_1)); //Also should be zero filled

    char title_str[13];
    memcpy(title_str, m_header.title, sizeof(m_header.title));
    title_str[12] = '\0';
    m_title = title_str;
}

auto GamePak::findSaveType() -> bool {
    for(size_t i = 0; i < m_rom.size(); i++) {
        char byte = static_cast<char>(m_rom[i]);

        if(byte == 'E' && (i + 5) < m_rom.size()) {
            //TODO: More stuff to detect size, possibly in EEPROM class
            const char next[6] = {static_cast<char>(m_rom[i + 1]),
                static_cast<char>(m_rom[i + 2]), static_cast<char>(m_rom[i + 3]), 
                static_cast<char>(m_rom[i + 4]), static_cast<char>(m_rom[i + 5]), '\0'};
            if(strcmp(next, "EPROM") == 0) {
                m_save = std::make_shared<EEPROM>(EEPROM_8K);
                LOG_INFO("EEPROM save type detected, assuming 8k");

                return true;
            }
        } else if(byte == 'S' && (i + 3) < m_rom.size()) {
            const char next[4] = {static_cast<char>(m_rom[i + 1]),
                static_cast<char>(m_rom[i + 2]), static_cast<char>(m_rom[i + 3]), '\0'};
            if(strcmp(next, "RAM") == 0) {
                m_save = std::make_shared<SRAM>();
                LOG_INFO("SRAM save type detected");

                return true;
            }
        } else if(byte == 'F' && (i + 4) < m_rom.size()) {
            const char next[5] = {static_cast<char>(m_rom[i + 1]),
                static_cast<char>(m_rom[i + 2]), static_cast<char>(m_rom[i + 3]), 
                static_cast<char>(m_rom[i + 4]), '\0'};
            if(strcmp(next, "LASH") == 0) {
                if((i + 5) < m_rom.size()) {
                    char size = static_cast<char>(m_rom[i + 5]);

                    if(size == '5') {
                        m_save = std::make_shared<Flash>(FLASH_64K);
                        LOG_INFO("Flash 64k save type detected");
                    
                        return true;
                    } else if(size == '1') {
                        m_save = std::make_shared<Flash>(FLASH_128K);
                        LOG_INFO("Flash 128k save type detected");

                        return true;
                    } else {
                        m_save = std::make_shared<Flash>(FLASH_64K);
                        LOG_INFO("Flash save type detected, assuming 64k");

                        return true;
                    }
                } else {
                    m_save = std::make_shared<Flash>(FLASH_64K);
                    LOG_INFO("Flash save type detected, assuming 64k");

                    return true;
                }
            }
        }
    }

    return false;
}

} //namespace emu