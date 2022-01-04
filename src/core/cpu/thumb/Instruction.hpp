#pragma once

#include "common/Types.hpp"

#include <string>


namespace emu {

enum ThumbInstructionType {
    THUMB_MOVE_SHIFTED_REGISTER,
    THUMB_ADD_SUBTRACT,
    THUMB_PROCESS_IMMEDIATE,
    THUMB_ALU_OPERATION,
    THUMB_HI_REGISTER_OPERATION,
    THUMB_BRANCH_EXCHANGE,
    THUMB_PC_RELATIVE_LOAD,
    THUMB_LOAD_STORE_REGISTER,
    THUMB_LOAD_STORE_SIGN_EXTEND,
    THUMB_LOAD_STORE_IMMEDIATE,
    THUMB_LOAD_STORE_HALFWORD,
    THUMB_SP_RELATIVE_LOAD_STORE,
    THUMB_LOAD_ADDRESS,
    THUMB_ADJUST_STACK_POINTER,
    THUMB_PUSH_POP_REGISTERS,
    THUMB_LOAD_STORE_MULTIPLE,
    THUMB_CONDITIONAL_BRANCH,
    THUMB_SOFTWARE_INTERRUPT,
    THUMB_UNCONDITIONAL_BRANCH,
    THUMB_LONG_BRANCH,
    THUMB_INVALID
};

struct ThumbInstruction {
    u16 instruction;
    ThumbInstructionType type;
    std::string disassembly;
};


auto thumbDetermineType(u16 instruction) -> ThumbInstructionType;
auto thumbDecodeInstruction(u16 instruction, u32 address = 0) -> ThumbInstruction;

} //namespace emu