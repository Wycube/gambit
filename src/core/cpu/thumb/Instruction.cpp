#include "Instruction.hpp"
#include "Disassembly.hpp"
#include "common/Pattern.hpp"


namespace emu {

/*
 * Only 8 bits are needed to decode a 16-bit THUMB instruction: bits 15-8 (8).
 */
const char *THUMB_ENCODINGS[] = {
    "000>>xxx", //Move Shifted Register
    "00011xxx", //Add/Subtract Register (Register and Immediate)
    "001xxxxx", //Add/Subtract/Compare/Move Immediate
    "010000xx", //ALU Operation
    "010001>>", //Hi Register Operation
    "01000111", //Branch and Exchange
    "01001xxx", //PC-Relative Load
    "0101xx0x", //Load/Store Register Offset
    "0101xx1x", //Load/Store Sign-Extended Byte/Halfword
    "011xxxxx", //Load/Store Immediate Offset
    "1000xxxx", //Load/Store Halfword
    "1001xxxx", //SP-Relative Load/Store
    "1010xxxx", //Load Address
    "10110000", //Adjust Stack Pointer
    "1011x10x", //Push/Pop Registers
    "1100xxxx", //Load/Store Multiple
    "1101>>>x", //Conditional Branch
    "11011111", //Software Interrupt
    "11100xxx", //Unconditional Branch
    "1111xxxx"  //Long Branch with Link
};

auto thumbDetermineType(u16 instruction) -> ThumbInstructionType {
    u8 decoding_bits = instruction >> 8;

    return static_cast<ThumbInstructionType>(common::match_bits(decoding_bits, THUMB_ENCODINGS));
}

auto thumbDecodeInstruction(u16 instruction) -> ThumbInstruction {
    ThumbInstructionType type = thumbDetermineType(instruction);
    std::string disassembly;

    switch(type) {
        case MOVE_SHIFTED_REGISTER : disassembly = disassembleMoveShifted(instruction);
        break;
        case ADD_SUBTRACT : disassembly = disassembleAddSubtract(instruction);
        break;
        case PROCESS_IMMEDIATE : disassembly = disassembleProcessImmediate(instruction);
        break;
        case ALU_OPERATION : disassembly = disassembleALUOperation(instruction);
        break;
        case HI_REGISTER_OPERATION : disassembly = disassembleHiRegOperation(instruction);
        break;
        case BRANCH_EXCHANGE : disassembly = disassembleBranchExchange(instruction);
        break;
        case PC_RELATIVE_LOAD : disassembly = disassemblePCRelativeLoad(instruction);
        break;
        case LOAD_STORE_REGISTER : disassembly = disassembleLoadStoreRegister(instruction);
        break;
        case LOAD_STORE_SIGN_EXTEND : disassembly = disassembleLoadStoreSigned(instruction);
        break;
        case LOAD_STORE_IMMEDIATE : disassembly = disassembleLoadStoreImmediate(instruction);
        break;
        case LOAD_STORE_HALFWORD : disassembly = disassembleLoadStoreHalfword(instruction);
        break;
        case SP_RELATIVE_LOAD_STORE : disassembly = disassembleSPRelativeLoadStore(instruction);
        break;
        case LOAD_ADDRESS : disassembly = disassembleLoadAddress(instruction);
        break;
        case ADJUST_STACK_POINTER : disassembly = disassembleAdjustSP(instruction);
        break;
        case PUSH_POP_REGISTERS : disassembly = disassemblePushPopRegisters(instruction);
        break;
        case LOAD_STORE_MULTIPLE : disassembly = disassembleLoadStoreMultiple(instruction);
        break;
        case CONDITIONAL_BRANCH : disassembly = disassembleConditionalBranch(instruction);
        break;
        case SOFTWARE_INTERRUPT : disassembly = disassembleSoftwareInterrupt(instruction);
        break;
        case UNCONDITIONAL_BRANCH : disassembly = disassembleUnconditionalBranch(instruction);
        break;
        case LONG_BRANCH : disassembly = disassembleLongBranch(instruction);
        break;
        case UNDEFINED : disassembly = disassembleUndefined(instruction);
        break;
    }

    return ThumbInstruction{instruction, type, disassembly};
}

} //namespace emu