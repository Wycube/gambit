#include "GPIO.hpp"
#include "common/Log.hpp"


namespace emu {

//TODO: RTC IRQ stuff, datetime unused bits, actually return the time, and add functionality for setting the time (for UI and stuff)
//TODO: Only enable RTC on games that had it

static constexpr int RTC_REGISTER_SIZES[8] = {0, 1, 7, 3, 2, 3, 0, 1};

RTC::RTC() {
    reset();
}

void RTC::reset() {
    clock = false;
    select = false;
    serial_buffer = 0;
    buffer_size = 0;
    params_transferred = 0;
    selected_reg = 0;
    state = INIT_START;
    resetRegisters();
}

auto RTC::read(u8 port_dir) -> u8 {
    u8 value = 0;

    if(!(port_dir & 2) && state == PARAM_TRANSFER_READ && select) {
        switch(selected_reg) {
            case REG_CONTROL :
                value = (control >> buffer_size) & 1;
                break;
            case REG_DATETIME :
                value = params_transferred < 4 ? (date[params_transferred] >> buffer_size) & 1 : (time[params_transferred - 4] >> buffer_size) & 1;
                break;
            case REG_TIME :
                value = (time[params_transferred] >> buffer_size) & 1;
                break;
            case REG_ALARM1 :
            case REG_ALARM2 :
            case REG_FREE : value = 1; break;
        }

        // LOG_INFO("RTC: Read {}", value);

        if(++buffer_size == 8) {
            if(++params_transferred == RTC_REGISTER_SIZES[selected_reg]) {
                // LOG_INFO("RTC: Finished reading register");
                state = END;
            }
            
            buffer_size = 0;
        }
    }

    return (value << 1);
}

void RTC::write(u8 value, u8 port_dir) {
    clock = port_dir & 4 ? value & 4 : clock;
    select = port_dir & 1 ? value & 1 : select;

    switch(state) {
        case INIT_START :
            if(!clock && select) {
                state = INIT_READY;
            }
            break;
        case INIT_READY :
            if(clock) {
                state = ACCEPT_COMMAND;
                serial_buffer = 0;
                buffer_size = 0;
            }
            break;
        case ACCEPT_COMMAND :
            if((port_dir & 2) && select) {
                //MSB-first
                serial_buffer = (serial_buffer << 1) | ((value >> 1) & 1);
                buffer_size++;
            }

            if(buffer_size == 8) {
                // LOG_INFO("RTC: Write command {:08b}", serial_buffer);
                // LOG_INFO("RTC: Selected Register: {}", (serial_buffer >> 1) & 7);
                state = serial_buffer & 1 ? PARAM_TRANSFER_READ : PARAM_TRANSFER_WRITE;
                selected_reg = (serial_buffer >> 1) & 7;
                serial_buffer = 0;
                buffer_size = 0;
                params_transferred = 0;

                if(RTC_REGISTER_SIZES[selected_reg] == 0) {
                    if(selected_reg == REG_RESET) {
                        resetRegisters();
                    } else if(selected_reg == REG_IRQ) {
                        //TODO: Whatever this does
                    }
                    state = END;
                }
            }
            break;
        case PARAM_TRANSFER_READ : break;
        case PARAM_TRANSFER_WRITE :
            if((port_dir & 2) && select) {
                //LSB-first
                serial_buffer = (serial_buffer >> 1) | ((value & 2) << 6);
                buffer_size++;
            }

            if(buffer_size == 8) {
                // LOG_INFO("RTC: Write Parameter {:08b}", serial_buffer);

                switch(selected_reg) {
                    case REG_CONTROL :
                        control = serial_buffer & 0x6A;
                        break;
                    case REG_DATETIME :
                        if(params_transferred < 4) {
                            date[params_transferred] = serial_buffer;
                        } else {
                            time[params_transferred - 4] = serial_buffer;
                        }
                        break;
                    case REG_TIME :
                        time[params_transferred] = serial_buffer;
                        break;
                }

                if(++params_transferred == RTC_REGISTER_SIZES[selected_reg]) {
                    state = END;
                }

                serial_buffer = 0;
                buffer_size = 0;
            }
            break;
        case END :
            if(!clock) {
                state = INIT_START;
            }
            break;
    }
}

void RTC::resetRegisters() {
    control = 0;

    //Reset time to 0 hours, 0 minutes, 0 seconds
    std::memset(time, 0, sizeof(time));

    //Reset date to January 1st, 2000
    //Except it is a Saturday not a Monday
    date[0] = 0;
    date[1] = 1;
    date[2] = 1;
    date[3] = 0;
}

GPIO::GPIO() {
    reset();
}

void GPIO::reset() {
    rtc.reset();
    port_dir = 0;
    read_write = false;
}

auto GPIO::read8(u32 address) -> u8 {
    switch(address) {
        case 0xC4 : return rtc.read(port_dir);
        case 0xC6 : return port_dir;
        case 0xC8 : return read_write;
    }

    return 0;
}

void GPIO::write8(u32 address, u8 value) {
    switch(address) {
        case 0xC4 : rtc.write(value, port_dir); break;
        case 0xC6 : port_dir = value & 0xF; break;
        case 0xC8 : read_write = value & 1; break;
    }
}

auto GPIO::readable() -> bool {
    return read_write;
}

} //namespace emu