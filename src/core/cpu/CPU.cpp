#include "CPU.hpp"
#include "arm/Instruction.hpp"
#include "common/Log.hpp"


namespace emu {

CPU::CPU(Bus &bus) : m_bus(bus) {
    m_pc = 0x08000000;
}

auto CPU::get_register(u8 reg) -> u32& {
    reg &= 0xF;

    if(reg < 8) {
        return m_regs[reg];
    }

    if(reg == 15) {
        return m_pc;
    }

    switch(m_mode) {
        case MODE_USER :
        case MODE_SYSTEM : return m_regs[reg];
        case MODE_FIQ : return (reg < 13 ? m_fiq_regs[reg - 8] : m_banked_regs[reg - 13]);
        case MODE_IRQ : return (reg < 13 ? m_regs[reg] : m_banked_regs[reg - 11]);
        case MODE_SUPERVISOR : return (reg < 13 ? m_regs[reg] : m_banked_regs[reg - 9]);
        case MODE_ABORT : return (reg < 13 ? m_regs[reg] : m_banked_regs[reg - 7]);
        case MODE_UNDEFINED : return (reg < 13 ? m_regs[reg] : m_banked_regs[reg - 5]);
    }
}

void CPU::step() {
    u32 instruction = m_pipeline[0];

    m_pipeline[0] = m_pipeline[1];
    m_pipeline[1] = m_bus.read32(m_pc + 4);
    m_pc += 4;

    ArmInstruction decoded = armDecodeInstruction(instruction);

    LOG_INFO("PC: {:08X} | Instruction: {:08X} | Disassembly: {}", m_pc - 8, instruction, decoded.disassembly);
}

void CPU::loadPipeline() {
    m_pipeline[0] = m_bus.read32(m_pc);
    m_pipeline[1] = m_bus.read32(m_pc + 4);
    m_pc += 4;
}

} //namespace emu