#pragma once

#include "save/Save.hpp"
#include "common/Types.hpp"
#include <vector>
#include <memory>
#include <string>


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

class GamePak final {
public:

    template<typename T>
    auto read(u32 address) -> T;
    template<typename T>
    void write(u32 address, T value);
    
    auto getHeader() -> GamePakHeader&;
    auto getTitle() -> const std::string&;
    auto size() -> u32;
    void loadROM(std::vector<u8> &&rom);

private:

    std::vector<u8> m_rom;
    GamePakHeader m_header;
    std::string m_title;
    std::unique_ptr<Save> m_save;

    void parseHeader();
    auto findSaveType() -> bool;
};

} //namespace emu