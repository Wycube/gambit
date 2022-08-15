#include "GBA.hpp"


namespace emu {

GBA::GBA(VideoDevice &video_device, InputDevice &input_device) 
: m_video_device(video_device), m_input_device(input_device), m_keypad(m_input_device), m_timer(m_scheduler, m_bus), m_dma(m_scheduler, m_bus), 
m_ppu(m_video_device, m_scheduler, m_bus, m_dma), m_bus(m_scheduler, m_keypad, m_timer, m_dma, m_ppu), m_cpu(m_bus), m_debugger(m_bus) {
    m_scheduler.attachDebugger(m_debugger);
    m_cpu.attachDebugger(m_debugger);
    m_ppu.attachDebugger(m_debugger);
}

void GBA::reset() {
    m_scheduler.reset();
    m_keypad.reset();
    m_timer.reset();
    m_dma.reset();
    m_ppu.reset();
    m_bus.reset();
    m_cpu.reset();
}

void GBA::step() {
    if(m_dma.running()) {
        m_scheduler.runToNext();
        return;
    }

    m_cpu.step();
}

bool GBA::run(u32 cycles) {
    u64 target = m_scheduler.getCurrentTimestamp() + cycles;

    while(m_scheduler.getCurrentTimestamp() < target) {
        if(!m_debugger.running()) {
            break;
        }

        if(m_dma.running()) {
            if(m_scheduler.nextEventTime() < target) {
                m_scheduler.runToNext();
            } else {
                m_scheduler.step(target - m_scheduler.getCurrentTimestamp());
            }

            continue;
        }
         
        m_cpu.step();
        m_debugger.checkBreakpoints();
    }

    return m_debugger.running();
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

auto GBA::getVideoDevice() -> VideoDevice& {
    return m_video_device;
}

auto GBA::getInputDevice() -> InputDevice& {
    return m_input_device;
}

auto GBA::getDebugger() -> dbg::Debugger& {
    return m_debugger;
}

} //namespace emu