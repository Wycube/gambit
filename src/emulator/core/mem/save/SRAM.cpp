#include "SRAM.hpp"
#include <cstring>


namespace emu {

SRAM::SRAM() {
    m_data.resize(32_KiB);
    m_type = SRAM_32K;
    reset();
}

void SRAM::reset() {
    memset(m_data.data(), 0xFF, m_data.size());
}

auto SRAM::read(u32 address) -> u8 {
    return m_data[address & 0x7FFF];
}

void SRAM::write(u32 address, u8 value) {
    m_data[address & 0x7FFF] = value;
}

} //namespace emu