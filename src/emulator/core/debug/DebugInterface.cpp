#include "DebugInterface.hpp"
#include "emulator/core/GBA.hpp"
#include "common/Log.hpp"


namespace emu {

DebugInterface::DebugInterface(GBA &core) : core(core) {
    reset();
}

void DebugInterface::reset() {
    cpu_usage.clear();
    frame_start = 0;
    force_break = false;
}

void DebugInterface::forceBreak() {
    force_break = true;
}

auto DebugInterface::getBreakpoint(u32 address) -> Breakpoint {
    if(breakpoints.count(address) != 0) {
        return breakpoints[address];
    }

    return Breakpoint{0, false, nullptr};
}

void DebugInterface::setBreakpoint(u32 address, ConditionFunc condition) {
    //If a breakpoint already exists at that address, change it
    if(breakpoints.count(address) != 0) {
        Breakpoint &bp = breakpoints[address];
        bp.address = address;
        bp.enabled = true;
        bp.condition = condition;
    } else {
        breakpoints.emplace(std::make_pair(address, Breakpoint{address, true, condition}));
    }
}

void DebugInterface::removeBreakpoint(u32 address) {
    breakpoints.erase(address);
}

auto DebugInterface::isBreakpoint(u32 address) -> bool {
    return breakpoints.count(address) != 0;
}

void DebugInterface::enableBreakpoint(u32 address, bool enable) {
    if(breakpoints.count(address) != 0) {
        breakpoints[address].enabled = enable;
    }
}

auto DebugInterface::isBreakpointEnabled(u32 address) -> bool {
    if(breakpoints.count(address) != 0) {
        return breakpoints[address].enabled;
    }

    return false;
}

auto DebugInterface::getBreakpoints() -> std::vector<Breakpoint> {
    //Since an unordered_map is used, the vector will not be sorted
    std::vector<Breakpoint> copy(breakpoints.size());
    for(const auto &b : breakpoints) {
        copy.push_back(b.second);
    }
    

    return copy;
}

void DebugInterface::setCallback(std::function<void ()> &&callback) {
    on_break = callback;
}

auto DebugInterface::onStep() -> bool {
    u32 pc = core.cpu.state.pc - (core.cpu.state.cpsr.t ? 2 : 4);

    if(force_break) {
        force_break = false;
        if(on_break) { on_break(); }
        return true;
    }

    if(breakpoints.count(pc) != 0 && breakpoints[pc].enabled) {
        //Unconditional Breakpoints do not have a condition function defined
        Breakpoint &bkpt = breakpoints[pc];
        if(!bkpt.condition || bkpt.condition(core)) {
            if(on_break) { on_break(); }
            return true;
        }
    }

    return false;
}

auto DebugInterface::getCPUUsage() const -> const common::ThreadSafeRingBuffer<float, 100>& {
    return cpu_usage;
}

void DebugInterface::onVblank() {
    u64 total_cycles = core.scheduler.getCurrentTimestamp() - frame_start;
    cpu_usage.push((float)core.cycles_active / (float)total_cycles * 100.0f);
    frame_start = core.scheduler.getCurrentTimestamp();
    core.cycles_active = 0;
}

auto DebugInterface::getRegister(u8 reg, u8 mode) const -> u32 {
    assert(reg < 16 && "Requested invalid register!");

    if(mode == 0) {
        mode = core.cpu.state.cpsr.mode;
    }

    switch(mode) {
        case MODE_USER :
        case MODE_SYSTEM : return *core.cpu.state.banks[0][reg];
        case MODE_FIQ : return *core.cpu.state.banks[1][reg];
        case MODE_IRQ : return *core.cpu.state.banks[2][reg];
        case MODE_SUPERVISOR : return *core.cpu.state.banks[3][reg];
        case MODE_ABORT : return *core.cpu.state.banks[4][reg];
        case MODE_UNDEFINED : return *core.cpu.state.banks[5][reg];
        default : LOG_FATAL("Requesting a register from an invalid mode!");
    }
}

//void DebugInterface::setRegister(u8 reg, u8 mode, u32 value) {}

auto DebugInterface::getCurrentStatus() const -> StatusRegister {
    return core.cpu.state.cpsr;
}

auto DebugInterface::getSavedStatus(u8 mode) const -> StatusRegister {
    if(mode == 0) {
        mode = core.cpu.state.cpsr.mode;
    }

    switch(mode) {
        case MODE_USER :
        case MODE_SYSTEM : return core.cpu.state.cpsr;
        case MODE_FIQ : return core.cpu.state.spsr[0];
        case MODE_IRQ : return core.cpu.state.spsr[1];
        case MODE_SUPERVISOR : return core.cpu.state.spsr[2];
        case MODE_ABORT : return core.cpu.state.spsr[3];
        case MODE_UNDEFINED : return core.cpu.state.spsr[4];
        default : LOG_FATAL("Requesting an SPSR from an invalid mode!", core.cpu.state.pc, mode);
    }
}

} //namespace emu