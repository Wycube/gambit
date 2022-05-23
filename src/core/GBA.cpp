#include "GBA.hpp"


namespace emu {

GBA::GBA() : m_ppu(m_scheduler), m_bus(m_scheduler, m_ppu), m_cpu(m_bus), m_debugger(m_bus) {
    m_cpu.attachDebugger(m_debugger);
    m_ppu.attachDebugger(m_debugger);
}

void GBA::reset() {
    m_scheduler.reset();
    m_ppu.reset();
    m_bus.reset();
    m_cpu.reset();
}

void GBA::step(u32 cycles) {
    u32 start = m_scheduler.getCurrentTimestamp();

    while(m_scheduler.getCurrentTimestamp() < start + cycles) {
        m_cpu.step();
    }
}

auto GBA::getGamePak() -> GamePak& {
    return m_bus.getLoadedPak();
}

void GBA::loadROM(const std::vector<u8> &rom) {
    m_bus.loadROM(rom);
    m_cpu.loadPipeline();
}

void GBA::loadBIOS(const std::vector<u8> &bios) {
    m_bus.loadBIOS(bios);
}

auto GBA::getDebugger() -> dbg::Debugger& {
    return m_debugger;
}

} //namespace emu