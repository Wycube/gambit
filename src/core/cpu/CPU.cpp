#include "CPU.hpp"
#include "arm/Instruction.hpp"
#include "thumb/Instruction.hpp"
#include "common/Log.hpp"


namespace emu {

CPU::CPU(Bus &bus) : m_bus(bus) {
    reset();
}

//TODO: Do this better
auto CPU::get_reg_ref(u8 reg, u8 mode) -> u32& {
    reg &= 0xF;

    if(mode == 0) {
        mode = m_state.mode;
    }

    if(reg < 13) {
        if(reg > 7 && mode == MODE_FIQ) {
            return m_state.fiq_regs[reg - 8];
        }

        return m_state.regs[reg];
    }

    if(reg == 15) {
        return m_state.pc;
    }

    switch(mode) {
        case MODE_USER :
        case MODE_SYSTEM : return m_state.banked_regs[reg - 13];
        case MODE_FIQ : return m_state.banked_regs[reg - 11];
        case MODE_IRQ : return m_state.banked_regs[reg - 9];
        case MODE_SUPERVISOR : return m_state.banked_regs[reg - 7];
        case MODE_ABORT : return m_state.banked_regs[reg - 5];
        case MODE_UNDEFINED : return m_state.banked_regs[reg - 3];
        default : LOG_FATAL("Mode {:05X}, is not a valid mode!", mode);
    }
}

auto CPU::get_reg(u8 reg, u8 mode) -> u32 {
    return get_reg_ref(reg, mode);
}

void CPU::set_reg(u8 reg, u32 value, u8 mode) {
    get_reg_ref(reg, mode) = value;
}

auto CPU::get_spsr(u8 mode) -> u32& {
    if(mode == 0) {
        mode = m_state.mode;
    }

    switch(mode) {
        case MODE_USER :
        case MODE_SYSTEM : return m_state.cpsr;
        case MODE_FIQ : return m_state.spsr[0];
        case MODE_IRQ : return m_state.spsr[1];
        case MODE_SUPERVISOR : return m_state.spsr[2];
        case MODE_ABORT : return m_state.spsr[3];
        case MODE_UNDEFINED : return m_state.spsr[4];
        default : LOG_FATAL("Mode {:05X}, is not a valid mode!", mode);
    }
}

auto CPU::get_flag(Flag flag) -> bool {
    return m_state.cpsr & flag;
}

void CPU::set_flag(Flag flag, bool set) {
    m_state.cpsr &= ~flag;
    m_state.cpsr |= flag * set;
}

auto CPU::passed(u8 condition) -> bool {
    condition &= 0xF;

    switch(condition) {
        case EQ : return get_flag(FLAG_ZERO);
        case NE : return !get_flag(FLAG_ZERO);
        case CS : return get_flag(FLAG_CARRY);
        case CC : return !get_flag(FLAG_CARRY);
        case MI : return get_flag(FLAG_NEGATIVE);
        case PL : return !get_flag(FLAG_NEGATIVE);
        case VS : return get_flag(FLAG_OVERFLOW);
        case VC : return !get_flag(FLAG_OVERFLOW);
        case HI : return get_flag(FLAG_CARRY) && !get_flag(FLAG_ZERO);
        case LS : return !get_flag(FLAG_CARRY) || get_flag(FLAG_ZERO);
        case GE : return get_flag(FLAG_NEGATIVE) == get_flag(FLAG_OVERFLOW);
        case LT : return get_flag(FLAG_NEGATIVE) != get_flag(FLAG_OVERFLOW);
        case GT : return !get_flag(FLAG_ZERO) && (get_flag(FLAG_NEGATIVE) == get_flag(FLAG_OVERFLOW));
        case LE : return get_flag(FLAG_ZERO) || (get_flag(FLAG_NEGATIVE) != get_flag(FLAG_OVERFLOW));
        case AL : return true;
        case NV : return false; //reserved on armv4T
    }

    return false;
}

void CPU::change_mode(PrivilegeMode mode) {
    m_state.mode = mode;
    m_state.cpsr &= ~0x1F;
    m_state.cpsr |= mode;
}

auto CPU::mode_from_bits(u8 mode) -> PrivilegeMode {
    switch(mode) {
        case MODE_USER : return MODE_USER;
        case MODE_SYSTEM : return MODE_SYSTEM;
        case MODE_FIQ : return MODE_FIQ;
        case MODE_IRQ : return MODE_IRQ;
        case MODE_SUPERVISOR : return MODE_SUPERVISOR;
        case MODE_ABORT : return MODE_ABORT;
        case MODE_UNDEFINED : return MODE_UNDEFINED;
        default : LOG_FATAL("Mode {:05X}, is not a valid mode!", mode);
    }
}

//Returns true if the CPU is currently in a privileged mode (User is the only non-privileged mode though).
auto CPU::privileged() -> bool {
    return m_state.mode != MODE_USER;
}

void CPU::execute_arm(u32 instruction) {
    ArmInstructionType type = armDetermineType(instruction);

    switch(type) {
        case ARM_BRANCH_AND_EXCHANGE : armBranchExchange(instruction); break;
        case ARM_PSR_TRANSFER : armPSRTransfer(instruction); break;
        case ARM_DATA_PROCESSING : armDataProcessing(instruction); break;
        case ARM_MULTIPLY : armMultiply(instruction); break;
        case ARM_MULTIPLY_LONG : armMultiplyLong(instruction); break;
        case ARM_SINGLE_DATA_SWAP : armDataSwap(instruction); break;
        case ARM_HALFWORD_DATA_TRANSFER : armHalfwordTransfer(instruction); break;
        case ARM_SINGLE_DATA_TRANSFER : armSingleTransfer(instruction); break;
        case ARM_BLOCK_DATA_TRANSFER : armBlockTransfer(instruction); break;
        case ARM_BRANCH : armBranch(instruction); break;
        case ARM_SOFTWARE_INTERRUPT : armSoftwareInterrupt(instruction); break;
        default: armUnimplemented(instruction);
    }
}

void CPU::execute_thumb(u16 instruction) {
    ThumbInstructionType type = thumbDetermineType(instruction);

    switch(type) {
        case THUMB_MOVE_SHIFTED_REGISTER : thumbMoveShifted(instruction); break;
        case THUMB_ADD_SUBTRACT : thumbAddSubtract(instruction); break;
        case THUMB_PROCESS_IMMEDIATE : thumbProcessImmediate(instruction); break;
        case THUMB_ALU_OPERATION: thumbALUOperation(instruction); break;
        case THUMB_HI_REGISTER_OPERATION : thumbHiRegisterOp(instruction); break;
        case THUMB_BRANCH_EXCHANGE : thumbBranchExchange(instruction); break;
        case THUMB_PC_RELATIVE_LOAD : thumbPCRelativeLoad(instruction); break;
        case THUMB_LOAD_STORE_REGISTER : thumbLoadStoreRegister(instruction); break;
        case THUMB_LOAD_STORE_IMMEDIATE : thumbLoadStoreImmediate(instruction); break;
        case THUMB_LOAD_STORE_HALFWORD : thumbLoadStoreHalfword(instruction); break;
        case THUMB_SP_RELATIVE_LOAD_STORE : thumbSPRelativeLoadStore(instruction); break;
        case THUMB_LOAD_ADDRESS : thumbLoadAddress(instruction); break;
        case THUMB_ADJUST_STACK_POINTER : thumbAdjustSP(instruction); break;
        case THUMB_PUSH_POP_REGISTERS : thumbPushPopRegisters(instruction); break;
        case THUMB_LOAD_STORE_MULTIPLE : thumbLoadStoreMultiple(instruction); break;
        case THUMB_CONDITIONAL_BRANCH : thumbConditionalBranch(instruction); break;
        case THUMB_SOFTWARE_INTERRUPT : thumbSoftwareInterrupt(instruction); break;
        case THUMB_UNCONDITIONAL_BRANCH : thumbUnconditionalBranch(instruction); break;
        case THUMB_LONG_BRANCH : thumbLongBranch(instruction); break;
        default: thumbUnimplemented(instruction);
    }
}

void CPU::service_interrupt() {
    u16 IE = m_bus.debugRead16(0x04000200);
    u16 IF = m_bus.debugRead16(0x04000202);
    bool IME = m_bus.debugRead8(0x04000208) & 1;

    if(!IME || IE == 0 || IF == 0) {
        return;
    }

    //Check for any interrupts that are enabled and requested
    for(int i = 0; i < 14; i++) {
        bool enabled = (IE << i) & 1;
        bool request = (IF << i) & 1;

        if(enabled && request) {
            m_state.exec = EXEC_ARM;
            m_state.pc = 0x18;
            loadPipeline();
            break;
        }
    }
}

void CPU::reset() {
    m_state.exec = EXEC_ARM;
    m_state.cpsr = 0;
    change_mode(MODE_SYSTEM);
    memset(m_state.spsr, 0, sizeof(m_state.spsr));
    memset(m_state.regs, 0, sizeof(m_state.regs));
    set_reg(13, 0x03007F00);
    set_reg(13, 0x03007FA0, MODE_IRQ);
    set_reg(13, 0x03007FE0, MODE_SUPERVISOR);
    m_state.pc = 0x08000000;
}

void CPU::step() {
    service_interrupt();

    if(m_state.exec == EXEC_ARM) {
        u32 instruction = m_state.pipeline[0];

        m_state.pipeline[0] = m_state.pipeline[1];
        m_state.pipeline[1] = m_bus.read32(m_state.pc + 4);
        m_state.pc += 4;

        ArmInstruction decoded = armDecodeInstruction(instruction, m_state.pc - 8);

        LOG_INFO("PC: {:08X} | Instruction: {:08X} | Disassembly: {}", m_state.pc - 8, instruction, decoded.disassembly);
        execute_arm(instruction);
    } else if(m_state.exec == EXEC_THUMB) {
        u16 instruction = m_state.pipeline[0];

        m_state.pipeline[0] = m_state.pipeline[1];
        m_state.pipeline[1] = m_bus.read16(m_state.pc + 2);
        m_state.pc += 2;

        ThumbInstruction decoded = thumbDecodeInstruction(instruction, m_state.pc - 4, m_bus.debugRead16(m_state.pc - 6));

        LOG_INFO("PC: {:08X} | Instruction: {:04X} | Disassembly: {}", m_state.pc - 4, instruction, decoded.disassembly);
        execute_thumb(instruction);
    }
}

void CPU::loadPipeline() {
    if(m_state.exec == EXEC_ARM) {
        m_state.pipeline[0] = m_bus.read32(m_state.pc);
        m_state.pipeline[1] = m_bus.read32(m_state.pc + 4);
        m_state.pc += 4;
    } else if(m_state.exec == EXEC_THUMB) {
        m_state.pipeline[0] = m_bus.read16(m_state.pc);
        m_state.pipeline[1] = m_bus.read16(m_state.pc + 2);
        m_state.pc += 2;
    }
}

void CPU::attachDebugger(dbg::Debugger &debugger) {
    //debugger.attachCPURegisters(m_state.regs, &m_state.pc, &m_state.cpsr);
    debugger.attachCPUState(&m_state);
}

} //namespace emu