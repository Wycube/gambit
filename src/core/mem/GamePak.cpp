#include "GamePak.hpp"


namespace emu {

auto GamePak::read8(u32 address) -> u8 {
    if(address >= m_rom.size()) {
       return 0;
    }

    return m_rom[address];
}

void GamePak::write8(u32 address, u8 value) {
    //RAM
}

auto GamePak::getHeader() -> GamePakHeader& {
    return m_header;
}

auto GamePak::size() -> u32 {
    return m_rom.size();
}

void GamePak::loadROM(std::vector<u8> &&rom) {
    m_rom = rom;
    parseHeader();
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
}

} //namespace emu