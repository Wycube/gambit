#include "Debugger.hpp"
#include "core/mem/Bus.hpp"
#include "core/cpu/arm/Instruction.hpp"
#include "core/cpu/thumb/Instruction.hpp"
#include "core/cpu/CPU.hpp"


namespace emu {

namespace dbg {

Debugger::Debugger(Bus &bus) : m_bus(bus) { }

void Debugger::attachCPURegisters(u32 *regs, u32 *pc, u32 *cpsr, u32 *spsr) {
    m_cpu_regs = regs;
    m_cpu_pc = pc;
    m_cpu_cpsr = cpsr;
    m_cpu_spsr = spsr;
}

void Debugger::attachPPUMem(u8 &vram, u32 &framebuffer) {
    m_ppu_vram = &vram;
    m_ppu_framebuffer = &framebuffer;
}

auto Debugger::getCPURegister(u8 index) -> u32 {
    index &= 0xF;
    
    if(index == 15) {
        return *m_cpu_pc;
    }

    return m_cpu_regs[index];
}

auto Debugger::getCPUCurrentStatus() -> u32 {
    return *m_cpu_cpsr;
}

auto Debugger::getCPUSavedStatus() -> u32 {
    return *m_cpu_spsr;
}

auto Debugger::read8(u32 address) -> u8 {
    return m_bus.debugRead8(address);
}

auto Debugger::read16(u32 address) -> u16 {
    return m_bus.debugRead16(address);
}

auto Debugger::read32(u32 address) -> u32 {
    return m_bus.debugRead32(address);
}

auto Debugger::getFramebuffer() -> u32* {
    return m_ppu_framebuffer;
}

auto Debugger::armDisassembleAt(u32 address) -> std::string {
    ArmInstruction decoded = armDecodeInstruction(m_bus.debugRead32(address), address);

    return decoded.disassembly;
}

auto Debugger::thumbDisassembleAt(u32 address) -> std::string {
    ThumbInstruction decoded = thumbDecodeInstruction(m_bus.debugRead16(address), address, m_bus.debugRead16(address - 2));

    return decoded.disassembly;
}

auto Debugger::atBreakPoint() -> bool {
    bool thumb = (*m_cpu_cpsr >> 5) & 0x1;
    
    if(thumb) {
        return (*m_cpu_pc - 2) == m_break_point;
    } else {
        return (*m_cpu_pc - 4) == m_break_point;
    }
}

void Debugger::setBreakPoint(u32 address) {
    m_break_point = address;
}

auto Debugger::getBreakPoint() -> u32 {
    return m_break_point;
}

} //namespace dbg

} //namespace emu