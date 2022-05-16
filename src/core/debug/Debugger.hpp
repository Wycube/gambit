#pragma once

#include "common/Types.hpp"

#include <string>


namespace emu {

//Forward declare Bus
class Bus;

namespace dbg {

class Debugger {
private:

    u32 m_break_point = 0xFFFFFFFF;

    //Pointers to CPU registers
    u32 *m_cpu_regs; //R0-R14
    u32 *m_cpu_pc; //R15
    u32 *m_cpu_cpsr;
    u32 *m_cpu_spsr;

    //Pointer to PPU stuff
    u8 *m_ppu_vram;
    u32 *m_ppu_framebuffer;

    Bus &m_bus;

public:

    Debugger(Bus &bus);

    void attachCPURegisters(u32 *regs, u32 *pc, u32 *cpsr, u32 *spsr);
    void attachPPUMem(u8 &vram, u32 &framebuffer);
    auto getCPURegister(u8 index) -> u32;
    auto getCPUCurrentStatus() -> u32;
    auto getCPUSavedStatus() -> u32;

    auto read8(u32 address) -> u8;
    auto read16(u32 address) -> u16;
    auto read32(u32 address) -> u32;
    auto getFramebuffer() -> u32*;

    auto armDisassembleAt(u32 address) -> std::string;
    auto thumbDisassembleAt(u32 address) -> std::string;

    auto atBreakPoint() -> bool;
    void setBreakPoint(u32 address);
    auto getBreakPoint() -> u32;
};

} //namespace dbg

} //namespace emu