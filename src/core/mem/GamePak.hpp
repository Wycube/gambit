#pragma once

#include "common/Types.hpp"

#include <vector>


namespace emu {

struct GamePakHeader {
    u32 entry_point;
    u8 logo[156];
    u8 title[12];
    u8 game_code[4];
    u8 maker_code[2];
    u8 fixed;
    u8 unit_code;
    u8 device_type;
    u8 reserved_0[7];
    u8 version;
    u8 checksum;
    u8 reserved_1[2];
};

class GamePak {
private:

    std::vector<u8> m_rom;
    GamePakHeader m_header;

    void parse_header();

public:

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

    auto getHeader() -> GamePakHeader&;
    auto size() -> u32;
    void loadROM(std::vector<u8> &&rom);
};

} //namespace emu