#include "Debugger.hpp"
#include "emulator/core/mem/Bus.hpp"
#include "emulator/core/cpu/arm/Instruction.hpp"
#include "emulator/core/cpu/thumb/Instruction.hpp"
#include "emulator/core/Scheduler.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

namespace dbg {

Debugger::Debugger(Bus &bus) : m_bus(bus) { }

auto Debugger::read8(u32 address) -> u8 {
    return m_bus.debugRead8(address);
}

auto Debugger::read16(u32 address) -> u16 {
    return m_bus.debugRead16(address);
}

auto Debugger::read32(u32 address) -> u32 {
    return m_bus.debugRead32(address);
}

auto Debugger::armDisassembleAt(u32 address) -> std::string {
    return armDecodeInstruction(m_bus.debugRead32(address), address).disassembly;
}

auto Debugger::thumbDisassembleAt(u32 address) -> std::string {
    return thumbDecodeInstruction(m_bus.debugRead16(address), address, m_bus.debugRead16(address - 2)).disassembly;
}

void Debugger::attachCPUState(CPUState *state) {
    m_cpu_state = state;
}

auto Debugger::getCPURegister(u8 reg, u8 mode) -> u32 {
    reg &= 0xF;

    if(mode == 0) {
        mode = m_cpu_state->cpsr.mode;
    }

    if(reg < 13) {
        if(reg > 7 && mode == MODE_FIQ) {
            return m_cpu_state->fiq_regs[reg - 8];
        }

        return m_cpu_state->regs[reg];
    }

    if(reg == 15) {
        return m_cpu_state->pc;
    }

    switch(mode) {
        case MODE_USER :
        case MODE_SYSTEM : return m_cpu_state->banked_regs[reg - 13];
        case MODE_FIQ : return m_cpu_state->banked_regs[reg - 11];
        case MODE_IRQ : return m_cpu_state->banked_regs[reg - 9];
        case MODE_SUPERVISOR : return m_cpu_state->banked_regs[reg - 7];
        case MODE_ABORT : return m_cpu_state->banked_regs[reg - 5];
        case MODE_UNDEFINED : return m_cpu_state->banked_regs[reg - 3];
        default : return 0;
    }
}

auto Debugger::getCPUCPSR() -> u32 {
    return m_cpu_state->cpsr.asInt();
}

auto Debugger::getCPUSPSR(u8 mode) -> u32 {
    if(mode == 0) {
        mode = m_cpu_state->cpsr.mode;
    }

    switch(mode) {
        case MODE_USER :
        case MODE_SYSTEM : return m_cpu_state->cpsr.asInt();
        case MODE_FIQ : return m_cpu_state->spsr[0].asInt();
        case MODE_IRQ : return m_cpu_state->spsr[1].asInt();
        case MODE_SUPERVISOR : return m_cpu_state->spsr[2].asInt();
        case MODE_ABORT : return m_cpu_state->spsr[3].asInt();
        case MODE_UNDEFINED : return m_cpu_state->spsr[4].asInt();
        default : return 0;
    }
}

auto Debugger::getCPUMode() -> u8 {
    return m_cpu_state->cpsr.mode;
}

void Debugger::attachScheduler(std::vector<Event> *scheduler_events, u32 *scheduler_timestamp) {
    m_scheduler_events = scheduler_events;
    m_scheduler_timestamp = scheduler_timestamp;
}

auto Debugger::numEvents() -> u32 {
    return m_scheduler_events->size();
}

auto Debugger::getEventTag(u32 index) -> std::string {
    return m_scheduler_events->at(index).debug_tag;
}

auto Debugger::getEventCycles(u32 index) -> u32 {
    return m_scheduler_events->at(index).scheduled_timestamp - *m_scheduler_timestamp;
}

auto Debugger::getCurrentCycle() -> u32 {
    return *m_scheduler_timestamp;
}

auto Debugger::atBreakPoint() -> bool {
    if(m_cpu_state->cpsr.t) {
        return (m_cpu_state->pc - 2) == m_break_point;
    } else {
        return (m_cpu_state->pc - 4) == m_break_point;
    }
}

void Debugger::setBreakPoint(u32 address) {
    m_break_point = address;
}

auto Debugger::getBreakPoint() -> u32 {
    return m_break_point;
}

} //namespace dbg

} //namespace emu