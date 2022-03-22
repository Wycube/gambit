#include "CPU.hpp"
#include "arm/Instruction.hpp"
#include "thumb/Instruction.hpp"
#include "common/Log.hpp"


namespace emu {

CPU::CPU(Bus &bus) : m_bus(bus) {
    m_cpsr = MODE_SYSTEM;
    m_exec = EXEC_ARM;
    m_regs[13] = 0x03007F00;
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

    u8 mode = m_cpsr & 0x1F;

    switch(mode) {
        case MODE_USER :
        case MODE_SYSTEM : return m_regs[reg];
        case MODE_FIQ : return (reg < 13 ? m_fiq_regs[reg - 8] : m_banked_regs[reg - 13]);
        case MODE_IRQ : return (reg < 13 ? m_regs[reg] : m_banked_regs[reg - 11]);
        case MODE_SUPERVISOR : return (reg < 13 ? m_regs[reg] : m_banked_regs[reg - 9]);
        case MODE_ABORT : return (reg < 13 ? m_regs[reg] : m_banked_regs[reg - 7]);
        case MODE_UNDEFINED : return (reg < 13 ? m_regs[reg] : m_banked_regs[reg - 5]);
        default : LOG_FATAL("Mode {:05X}, is not a valid mode!", mode);
    }
}

auto CPU::get_reg(u8 reg) -> u32 {
    return get_register(reg);
}

void CPU::set_reg(u8 reg, u32 value) {
    get_register(reg) = value;
}

auto CPU::get_flag(Flag flag) -> bool {
    return m_cpsr & flag;
}

void CPU::set_flag(Flag flag, bool set) {
    m_cpsr &= ~flag;
    m_cpsr |= flag * set;
}

auto CPU::passed(u8 condition) -> bool {
    condition &= 0xF;
    bool passed = false;

    switch(condition) {
        case 0x0 : passed = get_flag(FLAG_ZERO); //EQ
        break;
        case 0x1 : passed = !get_flag(FLAG_ZERO); //NE
        break;
        case 0x2 : passed = get_flag(FLAG_CARRY); //CS
        break;
        case 0x3 : passed = !get_flag(FLAG_CARRY); //CC
        break;
        case 0x4 : passed = get_flag(FLAG_NEGATIVE); //MI
        break;
        case 0x5 : passed = !get_flag(FLAG_NEGATIVE); //PL
        break;
        case 0x6 : passed = get_flag(FLAG_OVERFLOW); //VS
        break;
        case 0x7 : passed = !get_flag(FLAG_OVERFLOW); //VC
        break;
        case 0x8 : passed = get_flag(FLAG_CARRY) && !get_flag(FLAG_ZERO); //HI
        break;
        case 0x9 : passed = !get_flag(FLAG_CARRY) && get_flag(FLAG_ZERO); //LO
        break;
        case 0xA : passed = get_flag(FLAG_NEGATIVE) == get_flag(FLAG_OVERFLOW); //GE
        break;
        case 0xB : passed = get_flag(FLAG_NEGATIVE) != get_flag(FLAG_OVERFLOW); //LT
        break;
        case 0xC : passed = !get_flag(FLAG_ZERO) && (get_flag(FLAG_NEGATIVE) == get_flag(FLAG_OVERFLOW)); //GT
        break;
        case 0xD : passed = get_flag(FLAG_ZERO) && (get_flag(FLAG_NEGATIVE) != get_flag(FLAG_OVERFLOW)); //LE
        break;
        case 0xE : passed = true; //AL
        break;
        case 0xF : passed = false; //NV (reserved on armv4T)
        break;
    }

    return passed;
}

//Returns true if the CPU is currently in a privileged mode, and false otherwise (User is the only non-privileged mode though).
auto CPU::privileged() -> bool {
    u8 mode = m_cpsr & 0x1F;
    
    switch(mode) {
        case emu::MODE_FIQ :
        case emu::MODE_IRQ :
        case emu::MODE_SUPERVISOR :
        case emu::MODE_ABORT :
        case emu::MODE_UNDEFINED :
        case emu::MODE_SYSTEM : return true;
        default : return false;
    }
}

void CPU::execute_arm(u32 instruction) {
    ArmInstructionType type = armDetermineType(instruction);

    switch(type) {
        case ARM_BRANCH_AND_EXCHANGE : armBranchExchange(instruction); break;
        case ARM_PSR_TRANSFER : armPSRTransfer(instruction); break;
        case ARM_DATA_PROCESSING : armDataProcessing(instruction); break;
        case ARM_HALFWORD_DATA_TRANSFER : armHalfwordTransfer(instruction); break;
        case ARM_SINGLE_DATA_TRANSFER : armSingleTransfer(instruction); break;
        case ARM_BRANCH : armBranch(instruction); break;
        default: armUnimplemented(instruction); break;
    }
}

void CPU::execute_thumb(u16 instruction) {
    ThumbInstructionType type = thumbDetermineType(instruction);

    switch(type) {
        case THUMB_MOVE_SHIFTED_REGISTER : thumbMoveShifted(instruction); break;
        case THUMB_ADD_SUBTRACT : thumbAddSubtract(instruction); break;
        case THUMB_PROCESS_IMMEDIATE : thumbProcessImmediate(instruction); break;
        case THUMB_HI_REGISTER_OPERATION : thumbHiRegisterOp(instruction); break;
        case THUMB_PC_RELATIVE_LOAD : thumbPCRelativeLoad(instruction); break;
        case THUMB_PUSH_POP_REGISTERS : thumbPushPopRegisters(instruction); break;
        case THUMB_CONDITIONAL_BRANCH : thumbConditionalBranch(instruction); break;
        default: thumbUnimplemented(instruction);
    }
}

void CPU::step() {
    if(m_exec == EXEC_ARM) {
        u32 instruction = m_pipeline[0];

        m_pipeline[0] = m_pipeline[1];
        m_pipeline[1] = m_bus.read32(m_pc + 4);
        m_pc += 4;

        ArmInstruction decoded = armDecodeInstruction(instruction, m_pc - 8);

        LOG_INFO("PC: {:08X} | Instruction: {:08X} | Disassembly: {}", m_pc - 8, instruction, decoded.disassembly);
        execute_arm(instruction);
    } else if(m_exec == EXEC_THUMB) {
        u16 instruction = m_pipeline[0];

        m_pipeline[0] = m_pipeline[1];
        m_pipeline[1] = m_bus.read16(m_pc + 2);
        m_pc += 2;

        ThumbInstruction decoded = thumbDecodeInstruction(instruction);

        LOG_INFO("PC: {:08X} | Instruction: {:04X} | Disassembly: {}", m_pc - 4, instruction, decoded.disassembly);
        execute_thumb(instruction);
    }
}

void CPU::loadPipeline() {
    if(m_exec == EXEC_ARM) {
        m_pipeline[0] = m_bus.read32(m_pc);
        m_pipeline[1] = m_bus.read32(m_pc + 4);
        m_pc += 4;
    } else if(m_exec == EXEC_THUMB) {
        m_pipeline[0] = m_bus.read16(m_pc);
        m_pipeline[1] = m_bus.read16(m_pc + 2);
        m_pc += 2;
    }
}

void CPU::attachDebugger(dbg::Debugger &debugger) {
    debugger.attachCPURegisters(m_regs, &m_pc, &m_cpsr, &m_spsr);
}

} //namespace emu