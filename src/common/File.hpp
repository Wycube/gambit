#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <filesystem>


namespace common {

inline auto loadFileBytes(const char *path) -> std::vector<unsigned char> {
    if(!std::filesystem::exists(path)) {
        return {};
    }
    
    std::ifstream file(path, std::ios_base::binary);

    if(!file.is_open()) {
        return {};
    }

    size_t file_size = std::filesystem::file_size(path);
    std::vector<unsigned char> file_data(file_size);
    file.read((char*)file_data.data(), file_size);

    return file_data;
}

} //namespace common