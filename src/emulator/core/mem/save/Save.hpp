#pragma once

#include "common/Types.hpp"
#include <string>
#include <vector>


namespace emu {

enum SaveType {
    NONE,
    SRAM_32K,   //32KiB
    FLASH_64K,  //64KiB
    FLASH_128K, //128KiB
    EEPROM_512, //512 bytes
    EEPROM_8K   //8KiB
};

class Save {
public:

    virtual ~Save() { }

    virtual void reset() = 0;
    virtual auto read(u32 address) -> u8 = 0;
    virtual void write(u32 address, u8 value) = 0;
    void loadFromFile(const std::string &filename);
    void writeToFile(const std::string &filename);
    auto getType() -> SaveType;

protected:

    std::vector<u8> data;
    SaveType type;
};

class None final : public Save {
public:

    None() { type = NONE; }
    
    void reset() override { }
    auto read(u32 address) -> u8 override { return 0; }
    void write(u32 address, u8 value) override { }
};

} //namespace emu