#include "VBMLoader.hpp"
#include "common/Log.hpp"
#include <fstream>
#include <filesystem>


namespace movie {

auto loadVBMMovie(const std::string &path) -> VBMMovie {
    std::fstream file(path, std::ios::in | std::ios::binary);

    if(!file.is_open()) {
        LOG_FATAL("Failed to load VBM Movie '{}'!", path);
    }

    u8 expected[4] = {0x56, 0x42, 0x4D, 0x1A};
    u8 buffer[4] = {0};
    VBMMovie movie;

    file.read(reinterpret_cast<char*>(movie.signature), 4);
    file.read(reinterpret_cast<char*>(buffer), 4);
    movie.version = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
    file.read(reinterpret_cast<char*>(buffer), 4);
    movie.movie_uid = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
    file.read(reinterpret_cast<char*>(buffer), 4);
    movie.num_frames = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
    file.read(reinterpret_cast<char*>(buffer), 4);
    movie.rerecord_count = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
    file.read(reinterpret_cast<char*>(buffer), 1);
    movie.flags = buffer[0];
    file.read(reinterpret_cast<char*>(buffer), 1);
    movie.controller_flags = buffer[0];
    file.read(reinterpret_cast<char*>(buffer), 1);
    movie.system_flags = buffer[0];
    file.read(reinterpret_cast<char*>(buffer), 1);
    movie.emulator_flags = buffer[0];
    file.read(reinterpret_cast<char*>(buffer), 4);
    movie.win_save_type = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
    file.read(reinterpret_cast<char*>(buffer), 4);
    movie.win_flash_size = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
    file.read(reinterpret_cast<char*>(buffer), 4);
    movie.gb_emulator_type = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
    file.read(reinterpret_cast<char*>(movie.rom_title), 12);
    file.read(reinterpret_cast<char*>(buffer), 1);
    movie.vba_revision = buffer[0];
    file.read(reinterpret_cast<char*>(buffer), 1);
    movie.rom_crc = buffer[0];
    file.read(reinterpret_cast<char*>(buffer), 2);
    movie.crc_checksum = buffer[0] | (buffer[1] << 8);
    file.read(reinterpret_cast<char*>(buffer), 4);
    movie.game_code = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
    file.read(reinterpret_cast<char*>(buffer), 4);
    movie.state_offset = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
    file.read(reinterpret_cast<char*>(buffer), 4);
    movie.data_offset = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);

    file.read(reinterpret_cast<char*>(movie.info), 192);

    size_t size = std::filesystem::file_size(path);
    file.seekg(movie.data_offset);
    movie.inputs.resize(size - movie.data_offset);
    file.read(reinterpret_cast<char*>(movie.inputs.data()), size - movie.data_offset);

    LOG_INFO("Header Size: {}", sizeof(VBMMovie));
    LOG_INFO("Version: {}", movie.version);
    LOG_INFO("UID: {}", movie.movie_uid);
    LOG_INFO("Frames: {}", movie.num_frames);
    LOG_INFO("Actual Frames: {}", (size - movie.data_offset) / 2);
    LOG_INFO("Skip BIOS: {}", (movie.emulator_flags & 1) == 0 || (movie.emulator_flags & 2) == 2);
    LOG_INFO("State Offset: {}", movie.state_offset);
    LOG_INFO("Controller Data Offset: {}", movie.data_offset);
    LOG_INFO("Authors:\n{}", movie.info);
    LOG_INFO("Description:\n{}", &movie.info[64]);

    return movie;
}

} //namespace movie