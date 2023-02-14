#include "CPU.hpp"
#include "Names.hpp"
#include "arm/Instruction.hpp"
#include "thumb/Instruction.hpp"
#include "emulator/core/GBA.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

CPU::CPU(GBA &core) : core(core) {
    setupRegisterBanks();
    reset();
}

void CPU::reset() {
    state.halted = false;
    state.cpsr.fromInt(0);
    state.cpsr.mode = MODE_SYSTEM;
    std::memset(state.spsr, 0, sizeof(state.spsr));
    std::memset(state.regs, 0, sizeof(state.regs));
    setRegister(13, 0x03007F00);
    setRegister(13, 0x03007FA0, MODE_IRQ);
    setRegister(13, 0x03007FE0, MODE_SUPERVISOR);
    setRegister(14, 0x08000000);
    state.pc = 0x08000000;
}

void CPU::halt() {
    state.halted = true;
}

auto CPU::halted() -> bool {
    return state.halted;
}

void CPU::checkForInterrupt() {
    if(int_enable && int_flag.load()) {
        state.halted = false;
    }
}

void CPU::step() {
    if(service_interrupt()) {
        return;
    }

    if(!state.cpsr.t) {
        u32 instruction = state.pipeline[0];
        state.pipeline[0] = state.pipeline[1];
        state.pipeline[1] = core.bus.read32(state.pc + 4, SEQUENTIAL);
        state.pc += 4;

        // ArmInstruction decoded = armDecodeInstruction(instruction, state.pc - 8);
        // LOG_TRACE("PC: {:08X} | Instruction: {:08X}", state.pc - 8, instruction);

        if(passed(instruction >> 28)) {
            execute_arm(instruction);
        }
    } else {
        u16 instruction = state.pipeline[0];
        state.pipeline[0] = state.pipeline[1];
        state.pipeline[1] = core.bus.read16(state.pc + 2, SEQUENTIAL);
        state.pc += 2;

        // ThumbInstruction decoded = thumbDecodeInstruction(instruction, state.pc - 4, state.pipeline[0]);
        // LOG_TRACE("PC: {:08X} | Instruction: {:04X} | Disassembly: {}", state.pc - 4, instruction, decoded.disassembly);
        
        execute_thumb(instruction);
    }
}

//TODO: Proper pipeline timings N/S cycles
void CPU::flushPipeline() {
    if(!state.cpsr.t) {
        state.pipeline[0] = core.bus.read32(state.pc, NONSEQUENTIAL);
        state.pipeline[1] = core.bus.read32(state.pc + 4, SEQUENTIAL);
        state.pc += 4;
    } else {
        state.pipeline[0] = core.bus.read16(state.pc, NONSEQUENTIAL);
        state.pipeline[1] = core.bus.read16(state.pc + 2, SEQUENTIAL);
        state.pc += 2;
    }
}

auto CPU::readIO(u32 address) -> u8 {
    switch(address) {
        case 0x200 : return int_enable & 0xFF;
        case 0x201 : return int_enable >> 8;
        case 0x202 : return int_flag.load() & 0xFF;
        case 0x203 : return int_flag.load() >> 8;
        case 0x208 : return master_enable;
    }

    return 0;
}

void CPU::writeIO(u32 address, u8 value) {
    switch(address) {
        case 0x200 : int_enable = (int_enable & 0xFF00) | value; break;
        case 0x201 : int_enable = (int_enable & 0x00FF) | (value << 8); break;
        case 0x202 : int_flag.store(int_flag.load() & ~value); break;
        case 0x203 : int_flag.store(int_flag.load() & ~(value << 8)); break;
        case 0x208 : master_enable = value & 1;
    }
}

void CPU::requestInterrupt(InterruptSource source) {
    int_flag.store(int_flag.load() | source);
}

void CPU::setupRegisterBanks() {
    //r0-r12, same for all modes (except FIQ)
    for(size_t i = 0; i < 13; i++) {
        state.banks[0][i] = &state.regs[i];
        state.banks[1][i] = &state.regs[i];
        state.banks[2][i] = &state.regs[i];
        state.banks[3][i] = &state.regs[i];
        state.banks[4][i] = &state.regs[i];
        state.banks[5][i] = &state.regs[i];
    }

    //r8-r12 for FIQ
    state.banks[1][8] = &state.fiq_regs[0];
    state.banks[1][9] = &state.fiq_regs[1];
    state.banks[1][10] = &state.fiq_regs[2];
    state.banks[1][11] = &state.fiq_regs[3];
    state.banks[1][12] = &state.fiq_regs[4];

    //Mode specific r13 and r14
    state.banks[0][13] = &state.banked_regs[0];
    state.banks[0][14] = &state.banked_regs[1];
    state.banks[1][13] = &state.banked_regs[2];
    state.banks[1][14] = &state.banked_regs[3];
    state.banks[2][13] = &state.banked_regs[4];
    state.banks[2][14] = &state.banked_regs[5];
    state.banks[3][13] = &state.banked_regs[6];
    state.banks[3][14] = &state.banked_regs[7];
    state.banks[4][13] = &state.banked_regs[8];
    state.banks[4][14] = &state.banked_regs[9];
    state.banks[5][13] = &state.banked_regs[10];
    state.banks[5][14] = &state.banked_regs[11];

    //r15 the same for all modes
    state.banks[0][15] = &state.pc;
    state.banks[1][15] = &state.pc;
    state.banks[2][15] = &state.pc;
    state.banks[3][15] = &state.pc;
    state.banks[4][15] = &state.pc;
    state.banks[5][15] = &state.pc;
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
    }
}

auto CPU::service_interrupt() -> bool {
    //IE & IF
    u16 interrupts = int_enable & int_flag.load();

    if(!master_enable || interrupts == 0 || state.cpsr.i) {
        return false;
    }

    if(interrupts != 0) {
        //Sources
        LOG_TRACE("Interrupt serviced at PC: 0x{:08X}, Cycle: {}", state.pc, core.scheduler.getCurrentTimestamp());
        LOG_TRACE("Sources enabled and requested:");
        for(size_t i = 0; i < 14; i++) {
            if(bits::get_bit(interrupts, i)) {
                LOG_TRACE("\t- {}({})", interrupt_names[i], i);
            }
        }
        
        getSpsr(MODE_IRQ) = state.cpsr;
        state.cpsr.mode = MODE_IRQ;
        setRegister(14, state.cpsr.t ? state.pc + 2 : state.pc);
        state.cpsr.t = false;
        state.cpsr.i = true;
        state.pc = 0x18;
        flushPipeline();

        return true;
    }

    return false;
}

auto CPU::getRegister(u8 reg, u8 mode) -> u32 {
    assert(reg < 16 && "Requested invalid register!");

    if(mode == 0) {
        mode = state.cpsr.mode;
    }

    switch(mode) {
        case MODE_USER :
        case MODE_SYSTEM : return *state.banks[0][reg];
        case MODE_FIQ : return *state.banks[1][reg];
        case MODE_IRQ : return *state.banks[2][reg];
        case MODE_SUPERVISOR : return *state.banks[3][reg];
        case MODE_ABORT : return *state.banks[4][reg];
        case MODE_UNDEFINED : return *state.banks[5][reg];
        default : LOG_FATAL("Invalid mode at PC={:08X}", state.pc);
    }
}

void CPU::setRegister(u8 reg, u32 value, u8 mode) {
    assert(reg < 16 && "Requested invalid register!");

    //Automatically align PC
    if(reg == 15) {
        value = state.cpsr.t ? bits::align<u16>(value) : bits::align<u32>(value);
    }

    if(mode == 0) {
        mode = state.cpsr.mode;
    }

    //Apparently invalid modes do nothing on write
    switch(mode) {
        case MODE_USER :
        case MODE_SYSTEM : *state.banks[0][reg] = value; break;
        case MODE_FIQ : *state.banks[1][reg] = value; break;
        case MODE_IRQ : *state.banks[2][reg] = value; break;
        case MODE_SUPERVISOR : *state.banks[3][reg] = value; break;
        case MODE_ABORT : *state.banks[4][reg] = value; break;
        case MODE_UNDEFINED : *state.banks[5][reg] = value; break;
        default : LOG_FATAL("Invalid mode at PC={:08X}", state.pc);
    }
}

auto CPU::getSpsr(u8 mode) -> StatusRegister& {
    if(mode == 0) {
        mode = state.cpsr.mode;
    }

    switch(mode) {
        case MODE_USER :
        case MODE_SYSTEM : return state.cpsr;
        case MODE_FIQ : return state.spsr[0];
        case MODE_IRQ : return state.spsr[1];
        case MODE_SUPERVISOR : return state.spsr[2];
        case MODE_ABORT : return state.spsr[3];
        case MODE_UNDEFINED : return state.spsr[4];
        default : LOG_FATAL("PC: {:08X}, Invalid Mode: {:02X}", state.pc, mode);
    }
}

auto CPU::passed(u8 condition) const -> bool {
    condition &= 0xF;

    switch(condition) {
        case EQ : return state.cpsr.z;
        case NE : return !state.cpsr.z;
        case CS : return state.cpsr.c;
        case CC : return !state.cpsr.c;
        case MI : return state.cpsr.n;
        case PL : return !state.cpsr.n;
        case VS : return state.cpsr.v;
        case VC : return !state.cpsr.v;
        case HI : return state.cpsr.c && !state.cpsr.z;
        case LS : return !state.cpsr.c || state.cpsr.z;
        case GE : return state.cpsr.n == state.cpsr.v;
        case LT : return state.cpsr.n != state.cpsr.v;
        case GT : return !state.cpsr.z && (state.cpsr.n == state.cpsr.v);
        case LE : return state.cpsr.z || (state.cpsr.n != state.cpsr.v);
        case AL : return true;
        case NV : return false; //reserved on armv4T
    }

    return false;
}

auto CPU::modeFromBits(u8 mode) const -> PrivilegeMode {
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
    return state.cpsr.mode != MODE_USER;
}

} //namespace emu