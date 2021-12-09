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

enum InstructionType {
    DATA_PROCESSING,
    MULTIPLY,
    MULTIPLY_LONG,
    SINGLE_DATA_SWAP,
    BRANCH_AND_EXCHANGE,
    HALFWORD_DATA_TRANSFER,
    SINGLE_DATA_TRANSFER,
    BLOCK_DATA_TRANSFER,
    BRANCH,
    COPROCESSOR_DATA_TRANSFER,
    COPROCESSOR_DATA_OPERATION,
    COPROCESSOR_REGISTER_TRANSFER,
    SOFTWARE_INTERRUPT,
    UNDEFINED
};

struct ArmInstruction {
    u32 instruction;
    std::string dissasembly;
    ConditionCode condition;
    InstructionType type;
};


extern const char *ARM_ENCODINGS[];

auto determineType(u32 instruction) -> InstructionType;
auto decodeInstruction(u32 instruction) -> ArmInstruction;

} //namespace emu