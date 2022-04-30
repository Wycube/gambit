#include "GamePak.hpp"


namespace emu {

auto GamePak::read8(u32 address) -> u8 {
    //u32 adjusted = address & 0x1FFFFFE;

    //if(address >= m_rom.size()) {
    //    return 0;
    //}

    return m_rom[address];
}

void GamePak::write8(u32 address, u8 value) {
    //RAM
}

auto GamePak::size() -> u32 {
    return m_rom.size();
}

void GamePak::loadROM(const std::vector<u8> &rom) {
    m_rom = rom;
}

} //namespace emu