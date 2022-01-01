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
    return *m_cpu_cpsr;
}

auto Debugger::readByte(u32 address) -> u8 {
    return m_bus.read8(address);
}

auto Debugger::armDisassembleAt(u32 address) -> std::string {
    ArmInstruction decoded = armDecodeInstruction(m_bus.read32(address));

    return decoded.disassembly;
}

auto Debugger::thumbDisassembleAt(u32 address) -> std::string {
    ThumbInstruction decoded = thumbDecodeInstruction(m_bus.read16(address));

    return decoded.disassembly;
}

} //namespace dbg

} //namespace emu