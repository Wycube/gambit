#pragma once

#include "emulator/core/cpu/thumb/Instruction.hpp"
#include <lest/lest.hpp>


#define THUMB_DECODE_CASE(title, instruction, _type, _disassembly) \
    CASE(title) { \
        emu::ThumbInstruction decoded = emu::thumbDecodeInstruction(instruction); \
        EXPECT(decoded.type == _type); \
        EXPECT(decoded.disassembly == _disassembly); \
    }


const lest::test thumb_decode_tests[] = {
    THUMB_DECODE_CASE("Move Shifted Register", 0x0832, emu::THUMB_MOVE_SHIFTED_REGISTER, "lsr r2, r6, #0x20"),
    THUMB_DECODE_CASE("Add or Subtract Register", 0x1F55, emu::THUMB_ADD_SUBTRACT, "sub r5, r2, #5"),
    THUMB_DECODE_CASE("Add/Subtract/Compare/Move Immediate", 0x2055, emu::THUMB_PROCESS_IMMEDIATE, "mov r0, #0x55"),
    THUMB_DECODE_CASE("ALU Operation", 0x423C, emu::THUMB_ALU_OPERATION, "tst r4, r7"),
    THUMB_DECODE_CASE("Hi Register Operation", 0x4548, emu::THUMB_HI_REGISTER_OPERATION, "cmp r0, r9"),
    THUMB_DECODE_CASE("Branch and Exchange", 0x47A0, emu::THUMB_BRANCH_EXCHANGE, "blx r4"),
    THUMB_DECODE_CASE("PC-Relative Load", 0x4E4A, emu::THUMB_PC_RELATIVE_LOAD, "ldr r6, [pc, #0x128]"),
    THUMB_DECODE_CASE("Load/Store Register Offset", 0x5C4C, emu::THUMB_LOAD_STORE_REGISTER, "ldrb r4, [r1, r1]"),
    THUMB_DECODE_CASE("Load/Store Sign-Extended Byte/Halfword", 0x52BB, emu::THUMB_LOAD_STORE_SIGN_EXTEND, "strh r3, [r7, r2]"),
    THUMB_DECODE_CASE("Load/Store Immediate Offset", 0x7297, emu::THUMB_LOAD_STORE_IMMEDIATE, "strb r7, [r2, #0xa]"),
    THUMB_DECODE_CASE("Load/Store Halfword", 0x8BC7, emu::THUMB_LOAD_STORE_HALFWORD, "ldrh r7, [r0, #0x1e]"),
    THUMB_DECODE_CASE("SP-Relative Load/Store", 0x918F, emu::THUMB_SP_RELATIVE_LOAD_STORE, "str r1, [sp, #0x23c]"),
    THUMB_DECODE_CASE("Load Address", 0xA24F, emu::THUMB_LOAD_ADDRESS, "add r2, pc, #0x13c"),
    THUMB_DECODE_CASE("Adjust Stack Pointer", 0xB0B1, emu::THUMB_ADJUST_STACK_POINTER, "add sp, #-0xc4"),
    THUMB_DECODE_CASE("Push/Pop Registers", 0xB5A1, emu::THUMB_PUSH_POP_REGISTERS, "push {r0, r5, r7, lr}"),
    THUMB_DECODE_CASE("Load/Store Multiple", 0xCD1F, emu::THUMB_LOAD_STORE_MULTIPLE, "ldmia r5!, {r0, r1, r2, r3, r4}"),
    THUMB_DECODE_CASE("Conditional Branch", 0xD3CF, emu::THUMB_CONDITIONAL_BRANCH, "bcc #-0x5e"),
    THUMB_DECODE_CASE("Software Interrupt", 0xDF19, emu::THUMB_SOFTWARE_INTERRUPT, "swi #25"),
    THUMB_DECODE_CASE("Unconditional Branch", 0xE4EB, emu::THUMB_UNCONDITIONAL_BRANCH, "b #-0x626"),
    THUMB_DECODE_CASE("Long Branch with Link", 0xF7FF, emu::THUMB_LONG_BRANCH, "bl"),
    THUMB_DECODE_CASE("Undefined", 0xB880, emu::THUMB_UNDEFINED, "undefined")
};