#include "GamePak.hpp"


namespace emu {

auto GamePak::read8(u32 address) -> u8 {
    //u32 adjusted = address & 0x1FFFFFE;

    return m_rom[address];
}

void GamePak::write8(u32 address, u8 value) {
    //RAM
}

void GamePak::loadROM(const std::vector<u8> &rom) {
    m_rom = rom;
}

} //namespace emu