#pragma once

#include "common/Types.hpp"

#include <string>


namespace emu {

enum ConditionCode : u8 {
    EQ = 0,  //Equal
    NE = 1,  //Not Equal
    CS = 2,  //Carry Set
    CC = 3,  //Carry Clear
    MI = 4,  //Negative / Minus
    PL = 5,  //Positive / Plus
    VS = 6,  //Overflow
    VC = 7,  //No Overflow
    HI = 8,  //Unsigned Higher
    LS = 9,  //Unsigned Lower or Same
    GE = 10, //Signed Greater than or Equal
    LT = 11, //Signed Less than
    GT = 12, //Signed Greater than
    LE = 13, //Signed Less than or Equal
    AL = 14, //Always (unconditional)
    UNPREDICTABLE = 15 //Unpredictable on ARMv4
};

enum ArmInstructionType {
    ARM_BRANCH_AND_EXCHANGE,
    ARM_DATA_PROCESSING,
    ARM_MULTIPLY,
    ARM_MULTIPLY_LONG,
    ARM_SINGLE_DATA_SWAP,
    ARM_HALFWORD_DATA_TRANSFER,
    ARM_SINGLE_DATA_TRANSFER,
    ARM_BLOCK_DATA_TRANSFER,
    ARM_BRANCH,
    ARM_COPROCESSOR_DATA_TRANSFER,
    ARM_COPROCESSOR_DATA_OPERATION,
    ARM_COPROCESSOR_REGISTER_TRANSFER,
    ARM_SOFTWARE_INTERRUPT,
    ARM_UNDEFINED
};

struct ArmInstruction {
    u32 instruction;
    ArmInstructionType type;
    ConditionCode condition;
    std::string disassembly;
};


extern const char *ARM_ENCODINGS[];

auto armDetermineType(u32 instruction) -> ArmInstructionType;
auto armDecodeInstruction(u32 instruction) -> ArmInstruction;

} //namespace emu