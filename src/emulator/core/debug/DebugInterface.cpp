#include "DebugInterface.hpp"
#include "emulator/core/GBA.hpp"
#include "common/Log.hpp"


namespace emu {

DebugInterface::DebugInterface(GBA &core) : core(core) {
    reset();
}

void DebugInterface::reset() {
    cpu_usage.clear();
    frame_start_cycle = 0;
    frame_times.clear();
    frame_start_time = std::chrono::steady_clock::now();
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

auto DebugInterface::getFrameTimes() const -> const common::ThreadSafeRingBuffer<float, 100>& {
    return frame_times;
}

void DebugInterface::onVblank() {
    u64 total_cycles = core.scheduler.getCurrentTimestamp() - frame_start_cycle;
    cpu_usage.push((float)core.cycles_active / (float)total_cycles * 100.0f);
    frame_start_cycle = core.scheduler.getCurrentTimestamp();
    core.cycles_active = 0;

    auto now = std::chrono::steady_clock::now();
    frame_times.push((float)(now - frame_start_time).count() / 1'000'000.0f);
    frame_start_time = now;
}

} //namespace emu