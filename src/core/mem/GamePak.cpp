#include "GamePak.hpp"


namespace emu {

auto GamePak::read8(u32 address) -> u8 {
    if((address - 0x08000000) >= m_rom.size()) {
       return 0;
    }

    return m_rom[address - 0x08000000];
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