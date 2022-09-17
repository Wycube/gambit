#include "Flash.hpp"
#include "common/Log.hpp"


constexpr u8 CHIP_IDS[2][2] = {
    {0x32, 0x1B}, //Panasonic 64Kib Manufacturer and Device ID
    {0x62, 0x13}  //Sonya 128Kib Manufaturer and Device ID
};

namespace emu {

Flash::Flash(SaveType type) {
    if(type != FLASH_64K && type != FLASH_128K) {
        LOG_FATAL("Invalid SaveType: {}, for Flash!", type);
    }

    m_type = type;
    m_data.reserve(type == FLASH_64K ? 64_KiB : 128_KiB);
    reset();
}

void Flash::reset() {
    m_state = READY;
    m_chip_id_mode = false;
    m_bank_2 = false;
    m_erase_next = false;
    
    //Clear all bytes to 255
    memset(m_data.data(), 0xFF, m_data.size());
}

auto Flash::read(u32 address) -> u8 {
    address &= 0xFFFF;

    // LOG_INFO("Flash read from 0x0E00{:04X}", address);

    if(m_chip_id_mode && address <= 1) {
        return CHIP_IDS[m_type == FLASH_128K][address];
    }

    return m_bank_2 ? m_data[0x10000 + address] : m_data[address];
}

void Flash::write(u32 address, u8 value) {
    address &= 0xFFFF;

    switch(m_state) {
        case READY:
            if(address == 0x5555 && value == 0xAA) {
                m_state = CMD_1;
                // LOG_INFO("READY to CMD_1");
            }
            break;
        case CMD_1:
            if(address == 0x2AAA && value == 0x55) {
                m_state = CMD_2;
                // LOG_INFO("CMD_1 to CMD_2");
            }
            break;
        case CMD_2:
            // LOG_INFO("CMD_2 recieving command {:02X} at address 0x0E00{:04X}", value, address);

            if(m_erase_next && (address & 0xFFF) == 0) {
                if(value == ERASE_SECTOR) {
                    if(m_bank_2) {
                        memset(m_data.data() + address + 0x10000, 0xFF, 0x1000);
                    } else {
                        memset(m_data.data() + address, 0xFF, 0x1000);
                    }
                    m_erase_next = false;
                    m_state = READY;
                }
                break;
            }

            if(address == 0x5555) {
                //Read the command and do the stuff

                if(m_erase_next) {
                    if(value == ERASE_CHIP) {
                        memset(m_data.data(), 0xFF, m_data.size());
                        m_erase_next = false;
                        m_state = READY;
                    }
                    break;
                }

                if(value == ENTER_CHIP_ID) {
                    m_chip_id_mode = true;
                    m_state = READY;
                    break;
                }
                if(value == EXIT_CHIP_ID) {
                    m_chip_id_mode = false;
                    m_state = READY;
                    break;
                }

                if(value == PREPARE_WRITE) {
                    m_state = WRITE;
                    break;
                }

                if(value == SET_BANK) {
                    m_state = BANK;
                    break;
                }

                if(value == PREPARE_FOR_ERASE) {
                    m_state = READY;
                    m_erase_next = true;
                    break;
                }

                LOG_ERROR("Unknown Flash command {:02X}", value);
            }
            break;
        case WRITE:
            if(m_bank_2) {
                m_data[0x10000 + address] = value;
            } else {
                m_data[address] = value;
            }
            m_state = READY;
            break;
        case BANK:
            if(address == 0) {
                m_bank_2 = value;
                m_state = READY;
            }
            break;
    }
}

} //namespace emu