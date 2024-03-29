#pragma once

#include "emulator/core/cpu/Types.hpp"
#include "common/Buffer.hpp"
#include "common/Types.hpp"
#include <unordered_map>
#include <functional>
#include <chrono>


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

    void reset();

    void forceBreak();
    auto getBreakpoint(u32 address) -> Breakpoint;
    void setBreakpoint(u32 address, ConditionFunc condition = nullptr);
    void removeBreakpoint(u32 address);
    auto isBreakpoint(u32 address) -> bool;
    void enableBreakpoint(u32 address, bool enable);
    auto isBreakpointEnabled(u32 address) -> bool;
    auto getBreakpoints() -> std::vector<Breakpoint>;
    void setCallback(std::function<void ()> &&callback);
    auto onStep() -> bool;

    auto getCPUUsage() const -> const common::ThreadSafeRingBuffer<float, 100>&;
    auto getFrameTimes() const -> const common::ThreadSafeRingBuffer<float, 100>&;
    void onVblank();

    // auto getRegister(u8 reg, u8 mode = 0) const -> u32;
    // void setRegister(u8 reg, u8 mode, u32 value);
    // auto getCurrentStatus() const -> StatusRegister;
    // auto getSavedStatus(u8 mode) const -> StatusRegister;

private:

    GBA &core;
    std::unordered_map<u32, Breakpoint> breakpoints;
    std::function<void ()> on_break;
    bool force_break;
    common::ThreadSafeRingBuffer<float, 100> cpu_usage;
    u64 frame_start_cycle;
    common::ThreadSafeRingBuffer<float, 100> frame_times;
    std::chrono::time_point<std::chrono::steady_clock> frame_start_time;
};

} //namespace emu