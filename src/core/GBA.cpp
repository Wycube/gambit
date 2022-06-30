#include "GBA.hpp"


namespace emu {

GBA::GBA() : m_timer(m_scheduler), m_dma(m_scheduler, m_bus), m_ppu(m_scheduler, m_bus), m_bus(m_scheduler, m_keypad, m_timer, m_dma, m_ppu), m_cpu(m_bus), m_debugger(m_bus) {
    m_scheduler.attachDebugger(m_debugger);
    m_cpu.attachDebugger(m_debugger);
    m_ppu.attachDebugger(m_debugger);
}

void GBA::reset() {
    m_scheduler.reset();
    m_dma.reset();
    m_ppu.reset();
    m_bus.reset();
    m_cpu.reset();
}

void GBA::step(u32 cycles) {
    u32 start = m_scheduler.getCurrentTimestamp();

    if(m_dma.running()) {
        m_scheduler.step(cycles);
        return;
    }

    while(m_scheduler.getCurrentTimestamp() < start + cycles) {
        m_cpu.step();
    }
}

auto GBA::getGamePak() -> GamePak& {
    return m_bus.getLoadedPak();
}

void GBA::loadROM(std::vector<u8> &&rom) {
    m_bus.loadROM(std::move(rom));
    m_cpu.flushPipeline();
}

void GBA::loadBIOS(const std::vector<u8> &bios) {
    m_bus.loadBIOS(bios);
}

auto GBA::getKeypad() -> Keypad& {
    return m_keypad;
}

auto GBA::getDebugger() -> dbg::Debugger& {
    return m_debugger;
}

} //namespace emu