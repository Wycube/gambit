#include "DebugInterface.hpp"
#include "emulator/core/GBA.hpp"


namespace emu {

DebugInterface::DebugInterface(GBA &core) : core(core) { }

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

auto DebugInterface::onStep() -> bool {
    u32 pc = core.cpu.state.pc;

    if(breakpoints.count(pc) != 0 && breakpoints[pc].enabled) {
        //Unconditional Breakpoints do not have a condition function defined
        return breakpoints[pc].condition ? breakpoints[pc].condition(core) : true;
    }

    return false;
}

auto DebugInterface::getRegister(u8 reg, u8 mode) -> u32 {
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
        default : return 0; //Apparently invalid modes return 0
    }
}

//void DebugInterface::setRegister(u8 reg, u8 mode, u32 value) {}

} //namespace emu