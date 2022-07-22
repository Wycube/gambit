#pragma once

#include "emulator/core/cpu/Types.hpp"
#include "common/Types.hpp"
#include <string>
#include <vector>


namespace emu {

class Bus;
struct Event;


namespace dbg {

class Debugger {
public:

    Debugger(Bus &bus);

    auto read8(u32 address) -> u8;
    auto read16(u32 address) -> u16;
    auto read32(u32 address) -> u32;
    auto armDisassembleAt(u32 address) -> std::string;
    auto thumbDisassembleAt(u32 address) -> std::string;

    void attachCPUState(const CPUState *state);
    auto getCPURegister(u8 reg, u8 mode = 0) -> u32;
    auto getCPUCPSR() -> u32;
    auto getCPUSPSR(u8 mode = 0) -> u32;
    auto getCPUMode() -> u8;

    void attachScheduler(const std::vector<Event> *scheduler_events, const u32 *scheduler_timestamp);
    auto numEvents() -> u32;
    auto getEventTag(u32 index) -> std::string;
    auto getEventCycles(u32 index) -> u32;
    auto getCurrentCycle() -> u32;

    void attachPPU(const u8 *vram);
    void updateBgImages();
    auto getBgImage(int index) -> const u32*;

    void setRunning(bool run);
    auto running() -> bool;
    auto checkBreakpoints() -> bool;
    void addBreakpoint(u32 address);
    void removeBreakpoint(u32 address);
    auto getBreakpoints() -> const std::vector<u32>&;

private:

    bool m_running;
    std::vector<u32> m_breakpoints;

    //Pointer to CPU state
    const CPUState *m_cpu_state = nullptr;

    //Pointer to Scheduler stuff
    const std::vector<Event> *m_scheduler_events;
    const u32 *m_scheduler_timestamp;

    const u8 *m_ppu_vram;
    u32 *m_bg_map;

    Bus &m_bus;
};

} //namespace dbg

} //namespace emu