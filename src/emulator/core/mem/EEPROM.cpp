#include "EEPROM.hpp"
#include "common/Log.hpp"


namespace emu {

EEPROM::EEPROM() {
    reset();
}

void EEPROM::reset() {
    m_state = ACCEPT_COMMAND;
    m_address = 0;
    m_serial_buffer = 0;
    m_buffer_size = 0;
}

auto EEPROM::read() -> u16 {
    u8 data = 1;

    switch(m_state) {
        case READ_DUMMY :
            m_buffer_size++;
            if(m_buffer_size == 4) {
                m_state = READ;
                m_buffer_size = 0;
            }

            data = 0;
            break;
        case READ :
            m_buffer_size++;
            data = (m_mem[m_address] >> (64 - m_buffer_size)) & 1;

            if(m_buffer_size == 64) {
                m_state = ACCEPT_COMMAND;
                m_buffer_size = 0;
            }
            
            break;
        default : break;
    }

    return data;
}

void EEPROM::write(u16 value) {
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
            if(m_buffer_size == 14) {
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
                m_mem[m_address] = m_serial_buffer;
                m_state = WRITE_END;
                m_serial_buffer = 0;
                m_buffer_size = 0;
            }
            break;
        case READ_END :
        case WRITE_END :
            if(m_buffer_size == 1) {
                if(m_serial_buffer == 0) {
                    if(m_state == READ_END) {
                        m_state = READ_DUMMY;
                    } else {
                        m_state = ACCEPT_COMMAND;
                    }

                    m_serial_buffer = 0;
                    m_buffer_size = 0;
                } else {
                    m_serial_buffer = 0;
                    m_buffer_size = 0;
                }
            }
            break;
        default : break;
    }
}

} //namespace emu