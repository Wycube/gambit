#include "SRAM.hpp"
#include <cstring>


namespace emu {

SRAM::SRAM(const std::string &path) {
    openFile(path, 32_KiB);
    type = SRAM_32K;
    reset();
}

void SRAM::reset() { }

auto SRAM::read(u32 address) -> u8 {
    return readFile(address & 0x7FFF);
}

void SRAM::write(u32 address, u8 value) {
    writeFile(address & 0x7FFF, value);
}

} //namespace emu