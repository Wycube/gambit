#pragma once

#include "Types.hpp"
#include "emulator/core/Scheduler.hpp"
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

    explicit GamePak(Scheduler &scheduler);

    template<typename T>
    auto read(u32 address, AccessType access) -> T;
    template<typename T>
    void write(u32 address, T value, AccessType access);
    
    void updateWaitstates(u16 waitcnt);
    auto getHeader() -> const GamePakHeader&;
    auto getTitle() -> const std::string&;
    auto size() -> u32;
    auto loadFile(const std::string &path) -> bool;
    void unload();

private:

    void parseHeader();
    auto findSaveType(const std::string &path) -> bool;

    [[maybe_unused]] Scheduler &scheduler;
    u8 sram_waitstate;
    u32 ws0_n, ws0_s, ws1_n, ws1_s, ws2_n, ws2_s;
    std::vector<u8> rom;
    GamePakHeader header;
    std::string title;
    std::unique_ptr<Save> save;
};

} //namespace emu