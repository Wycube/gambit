#include "CPU.hpp"
#include "Names.hpp"
#include "arm/Instruction.hpp"
#include "thumb/Instruction.hpp"
#include "emulator/core/GBA.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

CPU::CPU(GBA &core) : m_core(core) {
    reset();
}

void CPU::reset() {
    //Setup register banks
    for(int i = 0; i < 16; i++) {
        m_state.banks[0][i] = &get_reg_ref(i, MODE_SYSTEM);
        m_state.banks[1][i] = &get_reg_ref(i, MODE_FIQ);
        m_state.banks[2][i] = &get_reg_ref(i, MODE_IRQ);
        m_state.banks[3][i] = &get_reg_ref(i, MODE_SUPERVISOR);
        m_state.banks[4][i] = &get_reg_ref(i, MODE_ABORT);
        m_state.banks[5][i] = &get_reg_ref(i, MODE_UNDEFINED);
    }

    m_state.halted = false;
    m_state.cpsr.fromInt(0);
    m_state.cpsr.mode = MODE_SYSTEM;
    std::memset(m_state.spsr, 0, sizeof(m_state.spsr));
    std::memset(m_state.regs, 0, sizeof(m_state.regs));
    set_reg(13, 0x03007F00);
    set_reg(13, 0x03007FA0, MODE_IRQ);
    set_reg(13, 0x03007FE0, MODE_SUPERVISOR);
    set_reg(14, 0x00000000);
    set_reg(15, 0x00000000);

    m_core.debugger.attachCPUState(&m_state);
}

void CPU::step() {
    service_interrupt();

    if(!m_state.cpsr.t) {
        u32 instruction = m_state.pipeline[0];
        m_state.pipeline[0] = m_state.pipeline[1];
        m_state.pipeline[1] = m_core.bus.read32(m_state.pc + 4);
        m_state.pc += 4;

        // ArmInstruction decoded = armDecodeInstruction(instruction, m_state.pc - 8);
        // LOG_TRACE("PC: {:08X} | Instruction: {:08X}", m_state.pc - 8, instruction);

        if(passed(instruction >> 28)) {
            execute_arm(instruction);
        }
    } else {
        u16 instruction = m_state.pipeline[0];
        m_state.pipeline[0] = m_state.pipeline[1];
        m_state.pipeline[1] = m_core.bus.read16(m_state.pc + 2);
        m_state.pc += 2;

        // ThumbInstruction decoded = thumbDecodeInstruction(instruction, m_state.pc - 4, m_bus.debugRead16(m_state.pc - 6));
        // LOG_TRACE("PC: {:08X} | Instruction: {:04X} | Disassembly: {}", m_state.pc - 4, instruction, decoded.disassembly);
        
        execute_thumb(instruction);
    }
}

void CPU::halt() {
    m_state.halted = true;
}

auto CPU::halted() -> bool {
    return m_state.halted;
}

void CPU::checkForInterrupt() {
    u16 IE = m_core.bus.debugRead16(0x04000200);
    u16 IF = m_core.bus.debugRead16(0x04000202);

    if(IE && IF) {
        m_state.halted = false;
    }
}

void CPU::flushPipeline() {
    if(!m_state.cpsr.t) {
        m_state.pipeline[0] = m_core.bus.read32(m_state.pc);
        m_state.pipeline[1] = m_core.bus.read32(m_state.pc + 4);
        m_state.pc += 4;
    } else {
        m_state.pipeline[0] = m_core.bus.read16(m_state.pc);
        m_state.pipeline[1] = m_core.bus.read16(m_state.pc + 2);
        m_state.pc += 2;
    }
}

auto CPU::get_reg_ref(u8 reg, u8 mode) -> u32& {
    reg &= 0xF;

    if(mode == 0) {
        mode = m_state.cpsr.mode;
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

auto CPU::get_reg_banked(u8 reg, u8 mode) -> u32& {
    reg &= 0xF;

    if(mode == 0) {
        mode = m_state.cpsr.mode;
    }

    switch(mode) {
        case MODE_USER :
        case MODE_SYSTEM : return *m_state.banks[0][reg];
        case MODE_FIQ : return *m_state.banks[1][reg];
        case MODE_IRQ : return *m_state.banks[2][reg];
        case MODE_SUPERVISOR : return *m_state.banks[3][reg];
        case MODE_ABORT : return *m_state.banks[4][reg];
        case MODE_UNDEFINED : return *m_state.banks[5][reg];
        default : LOG_INFO("PC: {:08X}, Invalid Mode: {:02X}", m_state.pc, mode); //LOG_FATAL("PC: {:08X}, Mode {:05X}, is not a valid mode!", m_state.pc, mode);
    }

    static u32 dummy;

    return dummy;
}

auto CPU::get_reg(u8 reg, u8 mode) -> u32 {
    return get_reg_banked(reg, mode);
    //return get_reg_ref(reg, mode);
}

void CPU::set_reg(u8 reg, u32 value, u8 mode) {
    //Automatically align PC
    if(reg == 15) {
        value = m_state.cpsr.t ? bits::align<u16>(value) : bits::align<u32>(value);
    }

    //get_reg_ref(reg, mode) = value;
    get_reg_banked(reg, mode) = value;
}

auto CPU::get_spsr(u8 mode) -> StatusRegister& {
    if(mode == 0) {
        mode = m_state.cpsr.mode;
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

auto CPU::passed(u8 condition) const -> bool {
    condition &= 0xF;

    switch(condition) {
        case EQ : return m_state.cpsr.z;
        case NE : return !m_state.cpsr.z;
        case CS : return m_state.cpsr.c;
        case CC : return !m_state.cpsr.c;
        case MI : return m_state.cpsr.n;
        case PL : return !m_state.cpsr.n;
        case VS : return m_state.cpsr.v;
        case VC : return !m_state.cpsr.v;
        case HI : return m_state.cpsr.c && !m_state.cpsr.z;
        case LS : return !m_state.cpsr.c || m_state.cpsr.z;
        case GE : return m_state.cpsr.n == m_state.cpsr.v;
        case LT : return m_state.cpsr.n != m_state.cpsr.v;
        case GT : return !m_state.cpsr.z && (m_state.cpsr.n == m_state.cpsr.v);
        case LE : return m_state.cpsr.z || (m_state.cpsr.n != m_state.cpsr.v);
        case AL : return true;
        case NV : return false; //reserved on armv4T
    }

    return false;
}

auto CPU::mode_from_bits(u8 mode) const -> PrivilegeMode {
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
auto CPU::privileged() const -> bool {
    return m_state.cpsr.mode != MODE_USER;
}

void CPU::execute_arm(u32 instruction) {
    ArmInstructionType type = armDetermineType(instruction);

    switch(type) {
        case ARM_BRANCH_AND_EXCHANGE : armBranchExchange(instruction); break;
        case ARM_PSR_TRANSFER : armPSRTransfer(instruction); break;
        case ARM_DATA_PROCESSING : armDataProcessing(instruction); break;
        case ARM_MULTIPLY : armMultiply(instruction); break;
        case ARM_MULTIPLY_LONG : armMultiplyLong(instruction); break;
        case ARM_SINGLE_DATA_SWAP : armSingleDataSwap(instruction); break;
        case ARM_HALFWORD_DATA_TRANSFER : armHalfwordTransfer(instruction); break;
        case ARM_SINGLE_DATA_TRANSFER : armSingleTransfer(instruction); break;
        case ARM_UNDEFINED : armUndefined(instruction); break;
        case ARM_BLOCK_DATA_TRANSFER : armBlockTransfer(instruction); break;
        case ARM_BRANCH : armBranch(instruction); break;
        case ARM_COPROCESSOR_DATA_TRANSFER :
        case ARM_COPROCESSOR_DATA_OPERATION :
        case ARM_COPROCESSOR_REGISTER_TRANSFER : armUndefined(instruction); break;
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
        case THUMB_LOAD_STORE_SIGN_EXTEND : thumbLoadStoreSigned(instruction); break;
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
        case THUMB_UNDEFINED : thumbUndefined(instruction); break;
        default: thumbUnimplemented(instruction);
    }
}

void CPU::service_interrupt() {
    u16 IE = m_core.bus.debugRead16(0x04000200);
    u16 IF = m_core.bus.debugRead16(0x04000202);
    bool IME = m_core.bus.debugRead8(0x04000208) & 1;

    if(!IME || IE == 0 || IF == 0 || m_state.cpsr.i) {
        return;
    }

    //Check for any interrupts that are enabled and requested
    for(int i = 0; i < 14; i++) {
        bool enabled = bits::get_bit(IE, i);
        bool request = bits::get_bit(IF, i);

        if(enabled && request) {
            LOG_TRACE("Interrupt serviced from source {}({}) at pc = 0x{:08X}", interrupt_names[i], i, m_state.pc);
            get_spsr(MODE_IRQ) = m_state.cpsr;
            m_state.cpsr.mode = MODE_IRQ;
            set_reg(14, m_state.cpsr.t ? get_reg(15) + 2 : get_reg(15));
            m_state.cpsr.t = false;
            m_state.cpsr.i = true;
            set_reg(15, 0x00000018);
            flushPipeline();
            break;
        }
    }
}

} //namespace emu