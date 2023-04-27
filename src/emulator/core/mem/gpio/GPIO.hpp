#pragma once

#include "common/Types.hpp"


namespace emu {

class RTC {
public:

    RTC();

    void reset();
    auto read(u8 port_dir) -> u8;
    void write(u8 value, u8 port_dir); 

private:

    void resetRegisters();
    
    bool clock = false;
    bool select = false;
    u8 serial_buffer;
    int buffer_size;
    int params_transferred;
    int selected_reg;

    u8 control;
    u8 date[4]; //Year / Month / Day of the Month / Day of the Week
    u8 time[3]; //Hour / Minute / Second

    enum CommandState {
        INIT_START,
        INIT_READY,
        ACCEPT_COMMAND,
        PARAM_TRANSFER_WRITE,
        PARAM_TRANSFER_READ,
        END
    } state;

    enum Register {
        REG_RESET,
        REG_CONTROL,
        REG_DATETIME,
        REG_TIME,
        REG_ALARM1,
        REG_ALARM2,
        REG_IRQ,
        REG_FREE
    };
};

class GPIO {
public:

    GPIO();

    void reset();
    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);
    auto readable() -> bool;

private:

    RTC rtc;
    u8 port_dir;
    bool read_write;
};

} //namespace emu