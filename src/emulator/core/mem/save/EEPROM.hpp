#pragma once

#include "Save.hpp"


namespace emu {

class EEPROM final : public Save {
public:

    explicit EEPROM(SaveType save_type);
    ~EEPROM() = default;

    void reset() override;
    auto read(u32 address) -> u8 override;
    void write(u32 address, u8 value) override;

private:

    enum CommandState {
        ACCEPT_COMMAND,
        READ_GET_ADDRESS,
        WRITE_GET_ADDRESS,
        WRITE_GET_DATA,
        READ_END,
        WRITE_END,
        READ_DUMMY,
        READ
    } state;

    u16 address_latch;
    u64 serial_buffer;
    int buffer_size;
    int bus_size;
};

} //namespace emu