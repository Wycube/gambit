#include "SRAM.hpp"


namespace emu {

SRAM::SRAM() {
    m_data.reserve(32_KiB);
    m_type = SRAM_32K;
}

void SRAM::reset() {
    memset(m_data.data(), 0, m_data.size());
}

auto SRAM::read(u32 address) -> u8 {
    if(address >= m_data.size()) {
        return 0;
    }

    return m_data[address];
}

void SRAM::write(u32 address, u8 value) {
    if(address >= m_data.size()) {
        return;
    }

    m_data[address] = value;
}

} //namespace emu