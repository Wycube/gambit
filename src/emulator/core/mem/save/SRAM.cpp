#include "SRAM.hpp"
#include <cstring>


namespace emu {

SRAM::SRAM() {
    data.resize(32_KiB);
    type = SRAM_32K;
    reset();
}

void SRAM::reset() {
    std::memset(data.data(), 0xFF, data.size());
}

auto SRAM::read(u32 address) -> u8 {
    return data[address & 0x7FFF];
}

void SRAM::write(u32 address, u8 value) {
    data[address & 0x7FFF] = value;
}

} //namespace emu