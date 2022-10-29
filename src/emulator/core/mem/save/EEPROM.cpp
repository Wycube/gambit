#include "EEPROM.hpp"
#include "common/Log.hpp"


namespace emu {

EEPROM::EEPROM(SaveType type) {
    if(type != EEPROM_512 && type != EEPROM_8K) {
        LOG_FATAL("Invalid SaveType: {}, for EEPROM!", type);
    }

    m_type = type;
    m_data.resize(type == EEPROM_512 ? 512 : 8192);
    m_bus_size = type == EEPROM_512 ? 6 : 14;
    reset();
}

void EEPROM::reset() {
    m_state = ACCEPT_COMMAND;
    m_address = 0;
    m_serial_buffer = 0;
    m_buffer_size = 0;
}

auto EEPROM::read(u32 address) -> u8 {
    u8 data = 1;

    switch(m_state) {
        case READ_DUMMY :
            data = 0;
            m_buffer_size++;
            if(m_buffer_size == 4) {
                m_state = READ;
                m_buffer_size = 0;
            }
            break;
        case READ :
            //MSB first
            data = (m_data[m_address * 8 + (m_buffer_size / 8)] >> (7 - m_buffer_size % 8)) & 1;
            m_buffer_size++;

            if(m_buffer_size == 64) {
                m_state = ACCEPT_COMMAND;
                m_buffer_size = 0;
            }
            break;
        default : break;
    }

    return data;
}

void EEPROM::write(u32 address, u8 value) {
    if(m_state == READ_DUMMY || m_state == READ) {
        return;
    }

    m_serial_buffer = (m_serial_buffer << 1) | (value & 1);
    m_buffer_size++;

    switch(m_state) {
        case ACCEPT_COMMAND :
            if(m_buffer_size == 2) {
                switch(m_serial_buffer) {
                    case 0 : m_state = ACCEPT_COMMAND; break;
                    case 1 : m_state = ACCEPT_COMMAND; break;
                    case 2 : m_state = WRITE_GET_ADDRESS; break;
                    case 3 : m_state = READ_GET_ADDRESS; break;
                }
                m_serial_buffer = 0;
                m_buffer_size = 0;
            }
            break;
        case READ_GET_ADDRESS :
        case WRITE_GET_ADDRESS :
            if(m_buffer_size == m_bus_size) {
                m_address = m_serial_buffer & 0x3FF;
                
                if(m_state == READ_GET_ADDRESS) {
                    m_state = READ_END;
                } else {
                    m_state = WRITE_GET_DATA;
                }
                
                m_serial_buffer = 0;
                m_buffer_size = 0;
            }
            break;
        case WRITE_GET_DATA :
            if(m_buffer_size == 64) {
                for(size_t i = 0; i < 8; i++) {
                    m_data[m_address * 8 + i] = (m_serial_buffer >> (7 - i) * 8) & 0xFF;
                }

                m_state = WRITE_END;
                m_serial_buffer = 0;
                m_buffer_size = 0;
            }
            break;
        case READ_END :
        case WRITE_END :
            if(m_buffer_size == 1) {
                if(m_state == READ_END) {
                    m_state = READ_DUMMY;
                } else {
                    m_state = ACCEPT_COMMAND;
                }

                m_serial_buffer = 0;
                m_buffer_size = 0;
            }
            break;
        default : break;
    }
}

} //namespace emu