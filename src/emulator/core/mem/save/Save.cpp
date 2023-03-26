#include "Save.hpp"
#include "common/Log.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>


namespace emu {

auto Save::getType() -> SaveType {
    return type;
}

void Save::openFile(const std::string &path, size_t size) {
    if(std::filesystem::exists(path) && std::filesystem::is_regular_file(path)) {
        size_t file_size = std::filesystem::file_size(path);
        
        if(file_size != size) {
            file.open(path, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
            std::filesystem::resize_file(path, size);
            LOG_DEBUG("Created new save file '{}'", path);
        } else {
            file.open(path, std::ios::binary | std::ios::in | std::ios::out);
            LOG_DEBUG("Opened save file '{}'", path);
        }
    } else {
        file.open(path, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        std::filesystem::resize_file(path, size);
        LOG_DEBUG("Created new save file '{}'", path);
    }
}

auto Save::readFile(u32 index) -> u8 {
    u8 value;
    file.seekg(index);
    file.read(reinterpret_cast<char*>(&value), 1);
    
    return value;
}

void Save::writeFile(u32 index, u8 value) {
    file.seekp(index);
    file.write(reinterpret_cast<char*>(&value), 1);
}

} //namespace emu