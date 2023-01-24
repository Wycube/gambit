#include "Debugger.hpp"
#include "emulator/core/cpu/arm/Instruction.hpp"
#include "emulator/core/cpu/thumb/Instruction.hpp"
#include "emulator/core/GBA.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

namespace dbg {

Debugger::Debugger(GBA &core) : core(core) { }

auto Debugger::read8(u32 address) -> u8 {
    return core.bus.debugRead8(address);
}

auto Debugger::read16(u32 address) -> u16 {
    return core.bus.debugRead16(address);
}

auto Debugger::read32(u32 address) -> u32 {
    return core.bus.debugRead32(address);
}

// auto Debugger::armDisassembleAt(u32 address) -> std::string {
//     return armDecodeInstruction(core.bus.debugRead32(address), address).disassembly;
// }

// auto Debugger::thumbDisassembleAt(u32 address) -> std::string {
//     return thumbDecodeInstruction(core.bus.debugRead16(address), address, core.bus.debugRead16(address - 2)).disassembly;
// }

// void Debugger::attachCPUState(const CPUState *state) {
//     cpu_state = state;
// }

// auto Debugger::getCPURegister(u8 reg, u8 mode) -> u32 {
//     reg &= 0xF;

//     if(mode == 0) {
//         mode = cpu_state->cpsr.mode;
//     }

//     if(reg < 13) {
//         if(reg > 7 && mode == MODE_FIQ) {
//             return cpu_state->fiq_regs[reg - 8];
//         }

//         return cpu_state->regs[reg];
//     }

//     if(reg == 15) {
//         return cpu_state->pc;
//     }

//     switch(mode) {
//         case MODE_USER :
//         case MODE_SYSTEM : return cpu_state->banked_regs[reg - 13];
//         case MODE_FIQ : return cpu_state->banked_regs[reg - 11];
//         case MODE_IRQ : return cpu_state->banked_regs[reg - 9];
//         case MODE_SUPERVISOR : return cpu_state->banked_regs[reg - 7];
//         case MODE_ABORT : return cpu_state->banked_regs[reg - 5];
//         case MODE_UNDEFINED : return cpu_state->banked_regs[reg - 3];
//         default : return 0;
//     }
// }

// auto Debugger::getCPUCPSR() -> u32 {
//     return cpu_state->cpsr.asInt();
// }

// auto Debugger::getCPUSPSR(u8 mode) -> u32 {
//     if(mode == 0) {
//         mode = cpu_state->cpsr.mode;
//     }

//     switch(mode) {
//         case MODE_USER :
//         case MODE_SYSTEM : return cpu_state->cpsr.asInt();
//         case MODE_FIQ : return cpu_state->spsr[0].asInt();
//         case MODE_IRQ : return cpu_state->spsr[1].asInt();
//         case MODE_SUPERVISOR : return cpu_state->spsr[2].asInt();
//         case MODE_ABORT : return cpu_state->spsr[3].asInt();
//         case MODE_UNDEFINED : return cpu_state->spsr[4].asInt();
//         default : return 0;
//     }
// }

// auto Debugger::getCPUMode() -> u8 {
//     return cpu_state->cpsr.mode;
// }

void Debugger::attachScheduler(const std::vector<Event> *scheduler_events, const u64 *scheduler_timestamp) {
    this->scheduler_events = scheduler_events;
    this->scheduler_timestamp = scheduler_timestamp;
}

auto Debugger::numEvents() -> u32 {
    return scheduler_events->size();
}

auto Debugger::getEventHandle(u32 index) -> u32 {
    return scheduler_events->at(index).handle;
}

auto Debugger::getEventCycles(u32 index) -> u64 {
    return scheduler_events->at(index).scheduled_timestamp - *scheduler_timestamp;
}

auto Debugger::getCurrentCycle() -> u64 {
    return *scheduler_timestamp;
}

// void Debugger::onBreak(const std::function<void ()> &callback) {
//     on_break = callback;
// }

// auto Debugger::checkBreakpoints() -> bool {
//     u32 pc = cpu_state->cpsr.t ? cpu_state->pc - 2 : cpu_state->pc - 4;

//     for(const auto &breakpoint : breakpoints) {
//         if(breakpoint == pc) {
//             if(on_break) on_break();
//             LOG_DEBUG("Hit breakpoint at pc={:08X}", pc);
//             return true;
//         }
//     }

//     return false;
// }

// void Debugger::addBreakpoint(u32 address) {
//     breakpoints.push_back(address);
// }

// void Debugger::removeBreakpoint(u32 address) {
//     for(size_t i = 0; i < breakpoints.size(); i++) {
//         if(breakpoints[i] == address) {
//             breakpoints.erase(breakpoints.begin() + i);
//             break;
//         }
//     }
// }

// auto Debugger::getBreakpoints() -> const std::vector<u32>& {
//     return breakpoints;
// }

// void Debugger::forceBreak() {
//     u32 pc = cpu_state->cpsr.t ? cpu_state->pc - 2 : cpu_state->pc - 4;

//     if(on_break) on_break();
//     LOG_DEBUG("Forced break at pc={:08X}", pc);
// }

} //namespace dbg

} //namespace emu