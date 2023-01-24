#pragma once

#include "emulator/core/cpu/Types.hpp"
#include "common/Types.hpp"
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <functional>


namespace emu {

class GBA;
struct Event;

namespace dbg {

class Debugger final {
public:

    Debugger(GBA &core);

    auto read8(u32 address) -> u8;
    auto read16(u32 address) -> u16;
    auto read32(u32 address) -> u32;
    // auto armDisassembleAt(u32 address) -> std::string;
    // auto thumbDisassembleAt(u32 address) -> std::string;

    // void attachCPUState(const CPUState *state);
    // auto getCPURegister(u8 reg, u8 mode = 0) -> u32;
    // auto getCPUCPSR() -> u32;
    // auto getCPUSPSR(u8 mode = 0) -> u32;
    // auto getCPUMode() -> u8;

    void attachScheduler(const std::vector<Event> *scheduler_events, const u64 *scheduler_timestamp);
    auto numEvents() -> u32;
    auto getEventHandle(u32 index) -> u32;
    auto getEventCycles(u32 index) -> u64;
    auto getCurrentCycle() -> u64;

    // void onBreak(const std::function<void ()> &callback);
    // auto checkBreakpoints() -> bool;
    // void addBreakpoint(u32 address);
    // void removeBreakpoint(u32 address);
    // auto getBreakpoints() -> const std::vector<u32>&;
    // void forceBreak();

private:

    std::vector<u32> breakpoints;
    std::function<void ()> on_break;

    //Pointer to CPU state
    // const CPUState *cpu_state = nullptr;

    //Pointer to Scheduler stuff
    const std::vector<Event> *scheduler_events;
    const u64 *scheduler_timestamp;

    GBA &core;
};

} //namespace dbg

} //namespace emu