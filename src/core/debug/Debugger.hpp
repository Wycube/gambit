#pragma once

#include "common/Types.hpp"
#include "core/cpu/Types.hpp"

#include <string>
#include <vector>


namespace emu {

class Bus;
struct Event;


namespace dbg {

class Debugger {
private:

    u32 m_break_point = 0xFFFFFFFF;

    //Pointer to CPU state
    CPUState *m_cpu_state = nullptr;

    //Pointer to PPU stuff
    u32 *m_ppu_framebuffer = nullptr;

    //Pointer to Scheduler stuff
    std::vector<Event> *m_scheduler_events;
    u32 *m_scheduler_timestamp;

    Bus &m_bus;

public:

    Debugger(Bus &bus);

    auto read8(u32 address) -> u8;
    auto read16(u32 address) -> u16;
    auto read32(u32 address) -> u32;
    auto armDisassembleAt(u32 address) -> std::string;
    auto thumbDisassembleAt(u32 address) -> std::string;

    void attachCPUState(CPUState *state);
    auto getCPURegister(u8 reg, u8 mode = 0) -> u32;
    auto getCPUCPSR() -> u32;
    auto getCPUSPSR(u8 mode = 0) -> u32;
    auto getCPUMode() -> PrivilegeMode;
    auto getCPUExec() -> ExecutionState;

    void attachPPUMem(u32 *framebuffer);
    auto getFramebuffer() -> u32*;

    void attachScheduler(std::vector<Event> *scheduler_events, u32 *scheduler_timestamp);
    auto numEvents() -> u32;
    auto getEventTag(u32 index) -> std::string;
    auto getEventCycles(u32 index) -> u32;

    auto atBreakPoint() -> bool;
    void setBreakPoint(u32 address);
    auto getBreakPoint() -> u32;
};

} //namespace dbg

} //namespace emu