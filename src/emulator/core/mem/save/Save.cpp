#include "Save.hpp"
#include "common/Log.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>


namespace emu {

//TODO: Validation on save files based on detected save type (size)
void Save::loadFromFile(const std::string &filename) {
    std::ifstream file(filename, std::ios_base::binary);

    if(!file.is_open()) {
        LOG_ERROR("Failed to open save file {} for reading!", filename);
        return;
    }

    size_t size = std::min(data.size(), std::filesystem::file_size(filename));
    file.read(reinterpret_cast<char*>(data.data()), size);
    file.close();

    LOG_INFO("Save file {} ({} bytes) loaded", filename, size);
}

void Save::writeToFile(const std::string &filename) {
    std::ofstream file(filename, std::ios_base::binary);

    if(!file.is_open()) {
        LOG_ERROR("Failed to open save file {} for writing!", filename);
        return;
    }

    size_t size = data.size();
    file.write(reinterpret_cast<char*>(data.data()), size);
    file.close();

    LOG_INFO("Save file {} ({} bytes) written", filename, size);
}

auto Save::getType() -> SaveType {
    return type;
}

} //namespace emu