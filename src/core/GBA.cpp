#include "GBA.hpp"


namespace emu {

GBA::GBA() {
    m_cpu.attachBus(&m_bus);
}

void GBA::loadROM(std::vector<u8> &rom) {
    m_bus.loadROM(rom);
}

} //namespace emu