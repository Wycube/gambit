#pragma once

#include "core/cpu/thumb/Disassembly.hpp"

#include <lest/lest.hpp>


const lest::test thumb_disassembly_tests[] = {
    CASE("Move Shifted Register Instructions") {
        EXPECT(emu::thumbDisassembleMoveShifted(0x0660) == "lsl r0, r4, #0x19");
        EXPECT(emu::thumbDisassembleMoveShifted(0x0832) == "lsr r2, r6, #0x20");
        EXPECT(emu::thumbDisassembleMoveShifted(0x145F) == "asr r7, r3, #0x11");
    },

    CASE("Add or Subtract Register Instructions") {
        EXPECT(emu::thumbDisassembleAddSubtract(0x1888) == "add r0, r1, r2");
        EXPECT(emu::thumbDisassembleAddSubtract(0x1F55) == "sub r5, r2, #5");
    },

    CASE("Add/Subtract/Compare/Move Immediate Instructions") {
        EXPECT(emu::thumbDisassembleProcessImmediate(0x2055) == "mov r0, #0x55");
        EXPECT(emu::thumbDisassembleProcessImmediate(0x2D21) == "cmp r5, #0x21");
        EXPECT(emu::thumbDisassembleProcessImmediate(0x3200) == "add r2, #0x0");
        EXPECT(emu::thumbDisassembleProcessImmediate(0x3E03) == "sub r6, #0x3");
    },

    CASE("ALU Operation Instructions") {
        EXPECT(emu::thumbDisassembleALUOperation(0x4010) == "and r0, r2");
        EXPECT(emu::thumbDisassembleALUOperation(0x404B) == "eor r3, r1");
        EXPECT(emu::thumbDisassembleALUOperation(0x4095) == "lsl r5, r2");
        EXPECT(emu::thumbDisassembleALUOperation(0x40F1) == "lsr r1, r6");
        EXPECT(emu::thumbDisassembleALUOperation(0x4115) == "asr r5, r2");
        EXPECT(emu::thumbDisassembleALUOperation(0x415B) == "adc r3, r3");
        EXPECT(emu::thumbDisassembleALUOperation(0x41A8) == "sbc r0, r5");
        EXPECT(emu::thumbDisassembleALUOperation(0x41D0) == "ror r0, r2");
        EXPECT(emu::thumbDisassembleALUOperation(0x423C) == "tst r4, r7");
        EXPECT(emu::thumbDisassembleALUOperation(0x4241) == "neg r1, r0");
        EXPECT(emu::thumbDisassembleALUOperation(0x42AC) == "cmp r4, r5");
        EXPECT(emu::thumbDisassembleALUOperation(0x42F6) == "cmn r6, r6");
        EXPECT(emu::thumbDisassembleALUOperation(0x431A) == "orr r2, r3");
        EXPECT(emu::thumbDisassembleALUOperation(0x4369) == "mul r1, r5");
        EXPECT(emu::thumbDisassembleALUOperation(0x439E) == "bic r6, r3");
        EXPECT(emu::thumbDisassembleALUOperation(0x43F9) == "mvn r1, r7");
    },

    CASE("Hi Register Operation Instructions") {
        EXPECT(emu::thumbDisassembleHiRegOperation(0x4479) == "add r1, r15");
        EXPECT(emu::thumbDisassembleHiRegOperation(0x4548) == "cmp r0, r9");
        EXPECT(emu::thumbDisassembleHiRegOperation(0x4665) == "mov r5, r12");
    },

    CASE("Branch and Exchange Instructions") {
        EXPECT(emu::thumbDisassembleBranchExchange(0x47A0) == "blx r4");
        EXPECT(emu::thumbDisassembleBranchExchange(0x4790) == "blx r2");
    },

    CASE("PC-Relative Load Instructions") {
        EXPECT(emu::thumbDisassemblePCRelativeLoad(0x4BB6) == "ldr r3, [pc, #0x2d8]");
        EXPECT(emu::thumbDisassemblePCRelativeLoad(0x4E4A) == "ldr r6, [pc, #0x128]");
    },

    CASE("Load/Store Register Offset Instructions") {
        EXPECT(emu::thumbDisassembleLoadStoreRegister(0x5153) == "str r3, [r2, r5]");
        EXPECT(emu::thumbDisassembleLoadStoreRegister(0x549D) == "strb r5, [r3, r2]");
        EXPECT(emu::thumbDisassembleLoadStoreRegister(0x58D1) == "ldr r1, [r2, r3]");
        EXPECT(emu::thumbDisassembleLoadStoreRegister(0x5C4C) == "ldrb r4, [r1, r1]");
    },

    CASE("Load/Store Sign-Extended Byte/Halfword Instructions") {
        EXPECT(emu::thumbDisassembleLoadStoreSigned(0x52BB) == "strh r3, [r7, r2]");
        EXPECT(emu::thumbDisassembleLoadStoreSigned(0x56E3) == "ldrsb r3, [r4, r3]");
        EXPECT(emu::thumbDisassembleLoadStoreSigned(0x5B68) == "ldrh r0, [r5, r5]");
        EXPECT(emu::thumbDisassembleLoadStoreSigned(0x5F8E) == "ldrsh r6, [r1, r6]");
    },

    CASE("Load/Store Immediate Offset Instructions") {
        EXPECT(emu::thumbDisassembleLoadStoreImmediate(0x83D8) == "str r0, [r3, #0x3c]");
        EXPECT(emu::thumbDisassembleLoadStoreImmediate(0x7297) == "strb r7, [r2, #0xa]");
        EXPECT(emu::thumbDisassembleLoadStoreImmediate(0x6856) == "ldr r6, [r2, #0x4]");
        EXPECT(emu::thumbDisassembleLoadStoreImmediate(0x7FCC) == "ldrb r4, [r1, #0x1f]");
    },

    CASE("Load/Store Halfword Instructions") {
        EXPECT(emu::thumbDisassembleLoadStoreHalfword(0x8588) == "strh r0, [r1, #0x2c]");
        EXPECT(emu::thumbDisassembleLoadStoreHalfword(0x8BC7) == "ldrh r7, [r0, #0x1e]");
    },

    CASE("SP-Relative Load/Store Instructions") {
        EXPECT(emu::thumbDisassembleSPRelativeLoadStore(0x918F) == "str r1, [sp, #0x23c]");
        EXPECT(emu::thumbDisassembleSPRelativeLoadStore(0x9DD6) == "ldr r5, [sp, #0x358]");
    },

    CASE("Load Address Instructions") {
        EXPECT(emu::thumbDisassembleLoadAddress(0xA24F) == "add r2, pc, #0x13c");
        EXPECT(emu::thumbDisassembleLoadAddress(0xA879) == "add r0, sp, #0x1e4");
    },

    CASE("Adjust Stack Pointer Instructions") {
        EXPECT(emu::thumbDisassembleAdjustSP(0xB022) == "add sp, #0x88");
        EXPECT(emu::thumbDisassembleAdjustSP(0xB0B1) == "add sp, #-0xc4");
    },

    CASE("Push/Pop Registers Instructions") {
        EXPECT(emu::thumbDisassemblePushPopRegisters(0xB44D) == "push {r0, r2, r3, r6}");
        EXPECT(emu::thumbDisassemblePushPopRegisters(0xB5A1) == "push {r0, r5, r7, lr}");
        EXPECT(emu::thumbDisassemblePushPopRegisters(0xBCB1) == "pop {r0, r4, r5, r7}");
        EXPECT(emu::thumbDisassemblePushPopRegisters(0xBD0A) == "pop {r1, r3, pc}");
    },

    CASE("Load/Store Multiple Instructions") {
        EXPECT(emu::thumbDisassembleLoadStoreMultiple(0xC3CA) == "stmia r3!, {r1, r3, r6, r7}");
        EXPECT(emu::thumbDisassembleLoadStoreMultiple(0xCD1F) == "ldmia r5!, {r0, r1, r2, r3, r4}");
    },

    CASE("Conditional Branch Instructions") {
        EXPECT(emu::thumbDisassembleConditionalBranch(0xD100) == "bne #0x4");
        EXPECT(emu::thumbDisassembleConditionalBranch(0xD3CF) == "bcc #0xffffffa2");
        EXPECT(emu::thumbDisassembleConditionalBranch(0xDB66) == "blt #0xd0");
    },

    CASE("Software Interrupt Instructions") {
        EXPECT(emu::thumbDisassembleSoftwareInterrupt(0xDF19) == "swi #25");
        EXPECT(emu::thumbDisassembleSoftwareInterrupt(0xDF00) == "swi #0");
    },

    CASE("Unconditional Branch Instructions") {
        EXPECT(emu::thumbDisassembleUnconditionalBranch(0xE4EB) == "b #0xfffff9da");
        EXPECT(emu::thumbDisassembleUnconditionalBranch(0xE3FD) == "b #0x7fe");
    },

    //Long Branch Link Tests will go here

    CASE("Invalid Instructions") {
        EXPECT(emu::thumbDisassembleInvalid(0xADD0) == "Invalid Instruction");
    }
};