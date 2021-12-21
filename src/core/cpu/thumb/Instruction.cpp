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
        case THUMB_MOVE_SHIFTED_REGISTER : disassembly = thumbDisassembleMoveShifted(instruction);
        break;
        case THUMB_ADD_SUBTRACT : disassembly = thumbDisassembleAddSubtract(instruction);
        break;
        case THUMB_PROCESS_IMMEDIATE : disassembly = thumbDisassembleProcessImmediate(instruction);
        break;
        case THUMB_ALU_OPERATION : disassembly = thumbDisassembleALUOperation(instruction);
        break;
        case THUMB_HI_REGISTER_OPERATION : disassembly = thumbDisassembleHiRegOperation(instruction);
        break;
        case THUMB_BRANCH_EXCHANGE : disassembly = thumbDisassembleBranchExchange(instruction);
        break;
        case THUMB_PC_RELATIVE_LOAD : disassembly = thumbDisassemblePCRelativeLoad(instruction);
        break;
        case THUMB_LOAD_STORE_REGISTER : disassembly = thumbDisassembleLoadStoreRegister(instruction);
        break;
        case THUMB_LOAD_STORE_SIGN_EXTEND : disassembly = thumbDisassembleLoadStoreSigned(instruction);
        break;
        case THUMB_LOAD_STORE_IMMEDIATE : disassembly = thumbDisassembleLoadStoreImmediate(instruction);
        break;
        case THUMB_LOAD_STORE_HALFWORD : disassembly = thumbDisassembleLoadStoreHalfword(instruction);
        break;
        case THUMB_SP_RELATIVE_LOAD_STORE : disassembly = thumbDisassembleSPRelativeLoadStore(instruction);
        break;
        case THUMB_LOAD_ADDRESS : disassembly = thumbDisassembleLoadAddress(instruction);
        break;
        case THUMB_ADJUST_STACK_POINTER : disassembly = thumbDisassembleAdjustSP(instruction);
        break;
        case THUMB_PUSH_POP_REGISTERS : disassembly = thumbDisassemblePushPopRegisters(instruction);
        break;
        case THUMB_LOAD_STORE_MULTIPLE : disassembly = thumbDisassembleLoadStoreMultiple(instruction);
        break;
        case THUMB_CONDITIONAL_BRANCH : disassembly = thumbDisassembleConditionalBranch(instruction);
        break;
        case THUMB_SOFTWARE_INTERRUPT : disassembly = thumbDisassembleSoftwareInterrupt(instruction);
        break;
        case THUMB_UNCONDITIONAL_BRANCH : disassembly = thumbDisassembleUnconditionalBranch(instruction);
        break;
        case THUMB_LONG_BRANCH : disassembly = thumbDisassembleLongBranch(instruction);
        break;
        case THUMB_UNDEFINED : disassembly = thumbDisassembleUndefined(instruction);
        break;
    }

    return ThumbInstruction{instruction, type, disassembly};
}

} //namespace emu