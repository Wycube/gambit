#pragma once

#include "Save.hpp"
#include "common/Types.hpp"


namespace emu {

class EEPROM final : public Save {
public:

    EEPROM(SaveType type);
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
    } m_state;

    u16 m_address;
    u64 m_serial_buffer;
    int m_buffer_size;
    int m_bus_size;
};

} //namespace emu