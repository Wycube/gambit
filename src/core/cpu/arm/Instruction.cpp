#include "Instruction.hpp"
#include "Disassembly.hpp"
#include "common/Pattern.hpp"

#include <bitset>


namespace emu {

/* 
 * Only 12 bits are needed to decode a 32-bit ARM instruction: bits 27-20 (8) + bits 7-4 (4).
 * There is some ambiguity between BX and a Data Processing instruction, but it should
 * be interpreted as a Branch and Exchange not a TEQ, so Branch and Exchange is put first.
 */
const char *ARM_ENCODINGS[] = {
    "000100100001", //Branch and Exchange
    "00<xxxxx>xx>", //Data Processing
    "000000xx1001", //Multiply
    "00001xxx1001", //Multiply Long
    "00010x001001", //Single Data Swap
    "000xxxxx1<<1", //Halfword Data Transfer
    "01>xxxxxxxx>", //Single Data Transfer
    "100xxxxxxxxx", //Block Data Transfer
    "101xxxxxxxxx", //Branch
    "110xxxxxxxxx", //Coprocessor Data Transfer
    "1110xxxxxxx0", //Coprocessor Data Operation
    "1110xxxxxxx1", //Coprocessor Register Transfer
    "1111xxxxxxxx"  //Software Interrupt
};

auto armDetermineType(u32 instruction) -> ArmInstructionType {
    u16 decoding_bits = (((instruction >> 16) & 0xFF0) | ((instruction >> 4) & 0xF));

    return static_cast<ArmInstructionType>(common::match_bits(decoding_bits, ARM_ENCODINGS));
}

auto armDecodeInstruction(u32 instruction) -> ArmInstruction {
    ArmInstructionType type = armDetermineType(instruction);
    ConditionCode condition = static_cast<ConditionCode>(instruction >> 28);
    std::string dissasembly;

    switch(type) {
        case ARM_DATA_PROCESSING : dissasembly = armDisassembleDataProcessing(instruction);
        break;
        case ARM_MULTIPLY : dissasembly = armDisassembleMultiply(instruction);
        break;
        case ARM_MULTIPLY_LONG : dissasembly = armDisassembleMultiplyLong(instruction);
        break;
        case ARM_SINGLE_DATA_SWAP : dissasembly = armDisassembleDataSwap(instruction);
        break;
        case ARM_BRANCH_AND_EXCHANGE : dissasembly = armDisassembleBranchExchange(instruction);
        break;
        case ARM_HALFWORD_DATA_TRANSFER : dissasembly = armDisassembleHalfwordTransfer(instruction);
        break;
        case ARM_SINGLE_DATA_TRANSFER : dissasembly = armDisassembleSingleTransfer(instruction);
        break;
        case ARM_BLOCK_DATA_TRANSFER : dissasembly = armDisassembleBlockTransfer(instruction);
        break;
        case ARM_BRANCH : dissasembly = armDisassembleBranch(instruction);
        break;
        case ARM_COPROCESSOR_DATA_TRANSFER : dissasembly = armDisassembleCoDataTransfer(instruction);
        break;
        case ARM_COPROCESSOR_DATA_OPERATION : dissasembly = armDisassembleCoDataOperation(instruction);
        break;
        case ARM_COPROCESSOR_REGISTER_TRANSFER : dissasembly = armDisassembleCoRegisterTransfer(instruction);
        break;
        case ARM_SOFTWARE_INTERRUPT : dissasembly = armDisassembleSoftwareInterrupt(instruction);
        break;
        case ARM_UNDEFINED : dissasembly = armDisassembleUndefined(instruction);
        break;
    }

    return ArmInstruction{instruction, type, condition, dissasembly};
}

} //namespace emu