#pragma once

#include "common/Types.hpp"


namespace emu {

class EEPROM {
public:

    EEPROM();

    void reset();

    auto read() -> u16;
    void write(u16 value);

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

    //There are 512-byte and 8KiB (8192-byte) EEPROMs
    u64 m_mem[1024];
    u16 m_address;
    u64 m_serial_buffer;
    int m_buffer_size;
};

} //namespace emu