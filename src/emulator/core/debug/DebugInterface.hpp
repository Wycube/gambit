#pragma once

#include "common/Types.hpp"
#include <unordered_map>
#include <functional>


namespace emu {

class GBA;

using ConditionFunc = std::function<bool (const GBA &core)>;

struct Breakpoint {
    u32 address;
    bool enabled;
    ConditionFunc condition;
};

class DebugInterface final {
public:

    explicit DebugInterface(GBA &core);

    auto getBreakpoint(u32 address) -> Breakpoint;
    void setBreakpoint(u32 address, ConditionFunc condition = nullptr);
    void removeBreakpoint(u32 address);
    auto isBreakpoint(u32 address) -> bool;
    void enableBreakpoint(u32 address, bool enable);
    auto isBreakpointEnabled(u32 address) -> bool;
    auto getBreakpoints() -> std::vector<Breakpoint>;
    auto onStep() -> bool;

    auto getRegister(u8 reg, u8 mode) -> u32;
    // void setRegister(u8 reg, u8 mode, u32 value);

private:

    GBA &core;
    std::unordered_map<u32, Breakpoint> breakpoints;
};

} //namespace emu