#pragma once

#include "common/Types.hpp"
#include <string>
#include <vector>


namespace movie {

struct VBMMovie {
    u8 signature[4];
    u32 version;
    u32 movie_uid;
    u32 num_frames;
    u32 rerecord_count;
    u8 flags;
    u8 controller_flags;
    u8 system_flags;
    u8 emulator_flags;
    u32 win_save_type;
    u32 win_flash_size;
    u32 gb_emulator_type;
    u8 rom_title[12];
    u8 vba_revision;
    u8 rom_crc;
    u16 crc_checksum;
    u32 game_code;
    u32 state_offset;
    u32 data_offset;
    u8 info[192];
    std::vector<u8> inputs;
};

auto loadVBMMovie(const std::string &path) -> VBMMovie;

} //namespace movie