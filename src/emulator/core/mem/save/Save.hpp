#pragma once

#include "common/Types.hpp"
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
    // void loadFromFile(const std::string &path);
    // void saveToFile(const std::string &path);
    auto getType() -> SaveType { return m_type; }

protected:

    std::vector<u8> m_data;
    SaveType m_type;
};

class NoSave final : public Save {
public:

    NoSave() { m_type = NONE; }
    
    void reset() override { }
    auto read(u32 address) -> u8 override { return 0; }
    void write(u32 address, u8 value) override { }
};

} //namespace emu