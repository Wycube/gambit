#include "EEPROM.hpp"
#include "common/Log.hpp"


namespace emu {

EEPROM::EEPROM(SaveType save_type, const std::string &path) {
    if(save_type != EEPROM_512 && save_type != EEPROM_8K) {
        LOG_FATAL("Invalid SaveType: {}, for EEPROM!", save_type);
    }

    type = save_type;
    openFile(path, type == EEPROM_512 ? 512 : 8192);
    bus_size = type == EEPROM_512 ? 6 : 14;
    reset();
}

void EEPROM::reset() {
    state = ACCEPT_COMMAND;
    address_latch = 0;
    serial_buffer = 0;
    buffer_size = 0;
}

auto EEPROM::read(u32 address) -> u8 {
    u8 data_out = 1;

    switch(state) {
        case READ_DUMMY :
            data_out = 0;
            buffer_size++;
            if(buffer_size == 4) {
                state = READ;
                buffer_size = 0;
            }
            break;
        case READ :
            //MSB first
            data_out = (readFile(address_latch * 8 + (buffer_size / 8)) >> (7 - buffer_size % 8)) & 1;
            buffer_size++;

            if(buffer_size == 64) {
                state = ACCEPT_COMMAND;
                buffer_size = 0;
            }
            break;
        default : break;
    }

    return data_out;
}

void EEPROM::write(u32 address, u8 value) {
    if(state == READ_DUMMY || state == READ) {
        return;
    }

    serial_buffer = (serial_buffer << 1) | (value & 1);
    buffer_size++;

    switch(state) {
        case ACCEPT_COMMAND :
            if(buffer_size == 2) {
                switch(serial_buffer) {
                    case 0 : state = ACCEPT_COMMAND; break;
                    case 1 : state = ACCEPT_COMMAND; break;
                    case 2 : state = WRITE_GET_ADDRESS; break;
                    case 3 : state = READ_GET_ADDRESS; break;
                }
                serial_buffer = 0;
                buffer_size = 0;
            }
            break;
        case READ_GET_ADDRESS :
        case WRITE_GET_ADDRESS :
            if(buffer_size == bus_size) {
                address_latch = serial_buffer & 0x3FF;
                
                if(state == READ_GET_ADDRESS) {
                    state = READ_END;
                } else {
                    state = WRITE_GET_DATA;
                }
                
                serial_buffer = 0;
                buffer_size = 0;
            }
            break;
        case WRITE_GET_DATA :
            if(buffer_size == 64) {
                for(size_t i = 0; i < 8; i++) {
                    writeFile(address_latch * 8 + i, (serial_buffer >> (7 - i) * 8) & 0xFF);
                }

                state = WRITE_END;
                serial_buffer = 0;
                buffer_size = 0;
            }
            break;
        case READ_END :
        case WRITE_END :
            if(buffer_size == 1) {
                if(state == READ_END) {
                    state = READ_DUMMY;
                } else {
                    state = ACCEPT_COMMAND;
                }

                serial_buffer = 0;
                buffer_size = 0;
            }
            break;
        default : break;
    }
}

} //namespace emu