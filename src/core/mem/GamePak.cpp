#include "GamePak.hpp"


namespace emu {

auto GamePak::read8(u32 address) -> u8 {
    return m_rom[address];
}

void GamePak::write8(u32 address, u8 value) {
    //RAM
}

void GamePak::loadROM(std::vector<u8> &rom) {
    m_rom = rom;
}

} //namespace emu