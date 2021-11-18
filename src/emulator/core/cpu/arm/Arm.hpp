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

/* 
 * x means 1 or 0
 * < means can't all be unset
 * i.e. << can be 01, 10, 11, but not 00
 * Only 12 bits are needed to decode a 32-bit ARM instruction
 * bits 27-20 (8) + bits 7-4 (4).
 * 
 * However there is some ambiguity between BX and a Data Processing instruction,
 * but it should be interpreted as a Branch and Exchange not a TEQ, so Branch and Exchange is put first.
 */
const std::string ARM_ENCODINGS[] = {
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

    "011xxxxxxxx1", //Undefined
    "100xxxxxxxxx", //Block Data Transfer
    "101xxxxxxxxx", //Branch
    "110xxxxxxxxx", //Coprocessor Data Transfer
    "1110xxxxxxx0", //Coprocessor Data Operation
    "1110xxxxxxx1", //Coprocessor Register Transfer
    "1111xxxxxxxx"  //Software Interrupt
};

enum InstructionType {
    DATA_PROCESSING, 
    MULTIPLY, 
    MULTIPLY_LONG, 
    SINGLE_DATA_SWAP, 
    BRANCH_AND_EXCHANGE, 
    HALFWORD_DATA_TRANSFER, 
    SINGLE_DATA_TRANSFER, 
    UNDEFINED,
    BLOCK_DATA_TRANSFER,
    BRANCH,
    COPROCESSOR_DATA_TRANSFER,
    COPROCESSOR_DATA_OPERATION,
    COPROCESSOR_REGISTER_TRANSFER,
    SOFTWARE_INTERRUPT,
    UNKNOWN
};

struct ArmInstruction {
    u32 instruction;
    std::string dissasembly;
    ConditionCode condition;
    InstructionType type;
};


auto determineType(u32 instruction) -> InstructionType;
auto parseInstruction(u32 instruction) -> ArmInstruction;

} //namespace emu