#include "GBA.hpp"


namespace emu {

GBA::GBA() : m_ppu(m_scheduler), m_bus(m_scheduler), m_cpu(m_bus), m_debugger(m_bus) {
    m_cpu.attachDebugger(m_debugger);
}

void GBA::step() {
    m_cpu.step();
}

void GBA::loadROM(const std::vector<u8> &rom) {
    m_bus.loadROM(rom);
    m_cpu.loadPipeline();
}

auto GBA::getGamePak() -> GamePak& {
    return m_bus.getLoadedPak();
}

auto GBA::getDebugger() -> dbg::Debugger& {
    return m_debugger;
}

} //namespace emu