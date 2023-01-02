#include "Flash.hpp"
#include "common/Log.hpp"


constexpr u8 CHIP_IDS[2][2] = {
    {0x32, 0x1B}, //Panasonic 64Kib Manufacturer and Device ID
    {0x62, 0x13}  //Sonya 128Kib Manufaturer and Device ID
};

namespace emu {

Flash::Flash(SaveType save_type) {
    if(save_type != FLASH_64K && save_type != FLASH_128K) {
        LOG_FATAL("Invalid SaveType: {}, for Flash!", save_type);
    }

    this->type = save_type;
    data.resize(type == FLASH_64K ? 64_KiB : 128_KiB);
    reset();
}

void Flash::reset() {
    state = READY;
    chip_id_mode = false;
    bank_2 = false;
    erase_next = false;
    
    //Clear all bytes to 255
    std::memset(data.data(), 0xFF, data.size());
}

auto Flash::read(u32 address) -> u8 {
    address &= 0xFFFF;

    // LOG_INFO("Flash read from 0x0E00{:04X}", address);

    if(chip_id_mode && address <= 1) {
        return CHIP_IDS[type == FLASH_128K][address];
    }

    return bank_2 ? data[0x10000 + address] : data[address];
}

void Flash::write(u32 address, u8 value) {
    address &= 0xFFFF;

    switch(state) {
        case READY:
            if(address == 0x5555 && value == 0xAA) {
                state = CMD_1;
                // LOG_INFO("READY to CMD_1");
            }
            break;
        case CMD_1:
            if(address == 0x2AAA && value == 0x55) {
                state = CMD_2;
                // LOG_INFO("CMD_1 to CMD_2");
            }
            break;
        case CMD_2:
            // LOG_INFO("CMD_2 recieving command {:02X} at address 0x0E00{:04X}", value, address);

            if(erase_next && (address & 0xFFF) == 0) {
                if(value == ERASE_SECTOR) {
                    if(bank_2) {
                        std::memset(data.data() + address + 0x10000, 0xFF, 0x1000);
                    } else {
                        std::memset(data.data() + address, 0xFF, 0x1000);
                    }
                    erase_next = false;
                    state = READY;
                }
                break;
            }

            if(address == 0x5555) {
                //Read the command and do the stuff

                if(erase_next) {
                    if(value == ERASE_CHIP) {
                        std::memset(data.data(), 0xFF, data.size());
                        erase_next = false;
                        state = READY;
                    }
                    break;
                }

                if(value == ENTER_CHIP_ID) {
                    chip_id_mode = true;
                    state = READY;
                    break;
                }
                if(value == EXIT_CHIP_ID) {
                    chip_id_mode = false;
                    state = READY;
                    break;
                }

                if(value == PREPARE_WRITE) {
                    state = WRITE;
                    break;
                }

                if(value == SET_BANK) {
                    state = BANK;
                    break;
                }

                if(value == PREPARE_FOR_ERASE) {
                    state = READY;
                    erase_next = true;
                    break;
                }

                LOG_ERROR("Unknown Flash command {:02X}", value);
            }
            break;
        case WRITE:
            if(bank_2) {
                data[0x10000 + address] = value;
            } else {
                data[address] = value;
            }
            state = READY;
            break;
        case BANK:
            if(address == 0) {
                bank_2 = value;
                state = READY;
            }
            break;
    }
}

} //namespace emu