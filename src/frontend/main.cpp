#include "common/Version.hpp"
#include "common/Types.hpp"
#include "common/Log.hpp"
#include "core/GBA.hpp"

#include <filesystem>
#include <fstream>
#include <vector>


int main(int argc, char *argv[]) {
    printf("Version: %s\n", common::GIT_DESC);
    printf("Commit: %s\n", common::GIT_COMMIT);
    printf("Branch: %s\n", common::GIT_BRANCH);

    if(argc < 2) {
        LOG_ERROR("No ROM file specified!");
        return -1;
    }

    std::fstream file(argv[1], std::ios_base::in | std::ios_base::binary);

    if(!file.good()) {
        LOG_ERROR("File not good!");
        return -1;
    }

    size_t size = std::filesystem::file_size(argv[1]);
    LOG_INFO("ROM Size: {}", size);

    std::vector<u8> rom;
    rom.resize(size);
    
    for(size_t i = 0; i < size; i++) {
        rom[i] = static_cast<u8>(file.get());
    }

    emu::GBA gba;

    gba.loadROM(rom);
    gba.step();
}