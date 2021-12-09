#include "Instruction.hpp"
#include "Disassembly.hpp"
#include "common/Pattern.hpp"

#include <bitset>


namespace emu {

/* 
 * There is some ambiguity between BX and a Data Processing instruction,
 * but it should be interpreted as a Branch and Exchange not a TEQ, so Branch and Exchange is put first.
 */
const char *ARM_ENCODINGS[16] = {
    "000100100001", //Branch and Exchange

    //Data Processing
    "000xxxxx0xx1", //Register Shift
    "000xxxxxxxx0", //Immediate Shift
    "001xxxxxxxxx", //Immediate
    
    "000000xx1001", //Multiply
    "00001xxx1001", //Multiply Long

    "00010x001001", //Single Data Swap
    "000xxxxx1<<1", //Halfword Data Transfer

    //Single Data Transfer
    "011xxxxxxxx0", //Register Offset
    "010xxxxxxxxx", //Immediate Offset

    "100xxxxxxxxx", //Block Data Transfer
    "101xxxxxxxxx", //Branch
    "110xxxxxxxxx", //Coprocessor Data Transfer
    "1110xxxxxxx0", //Coprocessor Data Operation
    "1110xxxxxxx1", //Coprocessor Register Transfer
    "1111xxxxxxxx"  //Software Interrupt
};

static InstructionType types[17] = {
    BRANCH_AND_EXCHANGE,
    DATA_PROCESSING, DATA_PROCESSING, DATA_PROCESSING,
    MULTIPLY, 
    MULTIPLY_LONG, 
    SINGLE_DATA_SWAP, 
    HALFWORD_DATA_TRANSFER, 
    SINGLE_DATA_TRANSFER, SINGLE_DATA_TRANSFER,
    BLOCK_DATA_TRANSFER,
    BRANCH,
    COPROCESSOR_DATA_TRANSFER,
    COPROCESSOR_DATA_OPERATION,
    COPROCESSOR_REGISTER_TRANSFER,
    SOFTWARE_INTERRUPT,
    UNDEFINED
};

auto determineType(u32 instruction) -> InstructionType {
    u16 decoding_bits = (((instruction >> 16) & 0xFF0) | ((instruction >> 4) & 0xF));

    return types[common::match_bits<u16>(decoding_bits, ARM_ENCODINGS)];
}

auto decodeInstruction(u32 instruction) -> ArmInstruction {
    InstructionType type = determineType(instruction);
    ConditionCode condition = static_cast<ConditionCode>(instruction >> 28);
    std::string dissasembly;

    switch(type) {
        case DATA_PROCESSING : dissasembly = disassembleDataProcessing(instruction);
        break;
        case MULTIPLY : dissasembly = disassembleMultiply(instruction);
        break;
        case MULTIPLY_LONG : dissasembly = disassembleMultiplyLong(instruction);
        break;
        case SINGLE_DATA_SWAP : dissasembly = disassembleDataSwap(instruction);
        break;
        case BRANCH_AND_EXCHANGE : dissasembly = disassembleBranchExchange(instruction);
        break;
        case HALFWORD_DATA_TRANSFER : dissasembly = disassembleHalfwordTransfer(instruction);
        break;
        case SINGLE_DATA_TRANSFER : dissasembly = disassembleSingleTransfer(instruction);
        break;
        case UNDEFINED : dissasembly = disassembleUndefined(instruction);
        break;
        case BLOCK_DATA_TRANSFER : dissasembly = disassembleBlockTransfer(instruction);
        break;
        case BRANCH : dissasembly = disassembleBranch(instruction);
        break;
        case COPROCESSOR_DATA_TRANSFER : dissasembly = disassembleCoDataTransfer(instruction);
        break;
        case COPROCESSOR_DATA_OPERATION : dissasembly = disassembleCoDataOperation(instruction);
        break;
        case COPROCESSOR_REGISTER_TRANSFER : dissasembly = disassembleCoRegisterTransfer(instruction);
        break;
        case SOFTWARE_INTERRUPT : dissasembly = disassembleSoftwareInterrupt(instruction);
        break;
        default: dissasembly = "Unknown";
    }

    return ArmInstruction{instruction, dissasembly, condition, type};
}

} //namespace emu