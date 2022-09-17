#pragma once

#include "Save.hpp"


namespace emu {

enum Command : u8 {
    ENTER_CHIP_ID = 0x90,
    EXIT_CHIP_ID = 0xF0,
    PREPARE_FOR_ERASE = 0x80,
    ERASE_CHIP = 0x10,
    ERASE_SECTOR = 0x30,
    PREPARE_WRITE = 0xA0,
    SET_BANK = 0xB0
};

class Flash final : public Save {
public:

    Flash(SaveType type);
    ~Flash() = default;

    void reset() override;
    auto read(u32 address) -> u8 override;
    void write(u32 address, u8 value) override;

private:

    enum CommandState {
        READY,
        CMD_1,
        CMD_2,
        WRITE,
        BANK
    } m_state;

    bool m_chip_id_mode;
    bool m_bank_2;
};

} //namespace emu