#include "GBA.hpp"


namespace emu {

GBA::GBA() : m_cpu(m_bus) { }

void GBA::step() {
    m_cpu.step();
}

void GBA::loadROM(std::vector<u8> &rom) {
    m_bus.loadROM(rom);
    m_cpu.loadPipeline();
}

} //namespace emu