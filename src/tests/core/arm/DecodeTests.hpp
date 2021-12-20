#pragma once

#include "core/cpu/arm/Instruction.hpp"

#include <lest/lest.hpp>


#define ARM_DECODE_CASE(title, instruction, _condition, _type, _disassembly) \
    CASE(title) { \
        emu::ArmInstruction decoded = emu::armDecodeInstruction(instruction); \
        EXPECT(decoded.condition == _condition); \
        EXPECT(decoded.type == _type); \
        EXPECT(decoded.disassembly == _disassembly); \
    }


const lest::test arm_decode_tests[] = {
    ARM_DECODE_CASE("Data Procesing", 0x70E73250, emu::VC, emu::DATA_PROCESSING, "rscvc r3, r7, r0, asr r2"),
    ARM_DECODE_CASE("Multiply", 0xE0090B9F, emu::AL, emu::MULTIPLY, "mul r9, r15, r11"),
    ARM_DECODE_CASE("Multiply Long", 0x10FCAF9D, emu::NE, emu::MULTIPLY_LONG, "smlalnes r10, r12, r13, r15"),
    ARM_DECODE_CASE("Single Data Swap", 0xC14C3099, emu::GT, emu::SINGLE_DATA_SWAP, "swpgtb r3, r9, [r12]"),
    ARM_DECODE_CASE("Branch and Exchange", 0xB12FFF1C, emu::LT, emu::BRANCH_AND_EXCHANGE, "bxlt r12"),
    ARM_DECODE_CASE("Halfword Data Transfer", 0xD15554DF, emu::LE, emu::HALFWORD_DATA_TRANSFER, "ldrlesb r5, [r5, #-0x4f]"),
    ARM_DECODE_CASE("Single Data Transfer", 0x37E7400D, emu::CC, emu::SINGLE_DATA_TRANSFER, "strccb r4, [r7, +r13]!"),
    ARM_DECODE_CASE("Block Data Transfer", 0xE9A29020, emu::AL, emu::BLOCK_DATA_TRANSFER, "stmib r2!, {r5, r12, r15}"),
    ARM_DECODE_CASE("Branch", 0x0B123FEE, emu::EQ, emu::BRANCH, "bleq #0x48ffc0"),
    ARM_DECODE_CASE("Software Interrupt", 0x1F07BFAD, emu::NE, emu::SOFTWARE_INTERRUPT, "swine #507821"),
    ARM_DECODE_CASE("Undefined", 0x66A38F9C, emu::VS, emu::UNDEFINED, "undefined")
};