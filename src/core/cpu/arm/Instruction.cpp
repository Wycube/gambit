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
constexpr char ARM_ENCODINGS[][13] = {
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

    return static_cast<ArmInstructionType>(common::const_match_bits<13, 13, ARM_ENCODINGS>(decoding_bits));
}

auto armDecodeInstruction(u32 instruction, u32 address) -> ArmInstruction {
    ArmInstructionType type = armDetermineType(instruction);
    ConditionCode condition = static_cast<ConditionCode>(instruction >> 28);
    std::string dissasembly = armDisassemblyFuncs[type](instruction, address);

    return ArmInstruction{instruction, type, condition, dissasembly};
}

} //namespace emu