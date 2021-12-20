#pragma once

#include "common/Types.hpp"

#include <string>


namespace emu {

enum ThumbInstructionType {
    MOVE_SHIFTED_REGISTER,
    ADD_SUBTRACT,
    PROCESS_IMMEDIATE,
    ALU_OPERATION,
    HI_REGISTER_OPERATION,
    BRANCH_EXCHANGE,
    PC_RELATIVE_LOAD,
    LOAD_STORE_REGISTER,
    LOAD_STORE_SIGN_EXTEND,
    LOAD_STORE_IMMEDIATE,
    LOAD_STORE_HALFWORD,
    SP_RELATIVE_LOAD_STORE,
    LOAD_ADDRESS,
    ADJUST_STACK_POINTER,
    PUSH_POP_REGISTERS,
    LOAD_STORE_MULTIPLE,
    CONDITIONAL_BRANCH,
    SOFTWARE_INTERRUPT,
    UNCONDITIONAL_BRANCH,
    LONG_BRANCH,
    UNDEFINED
};

struct ThumbInstruction {
    u16 instruction;
    ThumbInstructionType type;
    std::string disassembly;
};


extern const char *THUMB_ENCODINGS[];

auto thumbDetermineType(u16 instruction) -> ThumbInstructionType;
auto thumbDecodeInstruction(u16 instruction) -> ThumbInstruction;

} //namespace emu