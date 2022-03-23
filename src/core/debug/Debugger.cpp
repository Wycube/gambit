#include "Debugger.hpp"
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

auto Debugger::romSize() -> u32 {
    return m_bus.romSize();
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

auto Debugger::armDisassembleAt(u32 address) -> std::string {
    ArmInstruction decoded = armDecodeInstruction(m_bus.debugRead32(address), address);

    return decoded.disassembly;
}

auto Debugger::thumbDisassembleAt(u32 address) -> std::string {
    ThumbInstruction decoded = thumbDecodeInstruction(m_bus.debugRead16(address), address);

    return decoded.disassembly;
}

} //namespace dbg

} //namespace emu