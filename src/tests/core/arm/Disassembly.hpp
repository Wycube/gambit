#pragma once

#include "core/cpu/arm/Disassembly.hpp"

#include <lest/lest.hpp>
#include <string>


const lest::test arm_disassembly_tests[] = {
    CASE("Data Processing Instructions") {
        EXPECT(emu::disassembleDataProcessing(0x02012101) == "andeq r2, r1, #0x40000000");
        EXPECT(emu::disassembleDataProcessing(0x10330004) == "eornes r0, r3, r4");
        EXPECT(emu::disassembleDataProcessing(0x20458A81) == "subcs r8, r5, r1, lsl #0x15");
        EXPECT(emu::disassembleDataProcessing(0x306E0D18) == "rsbcc r0, r14, r8, lsl r13");
        EXPECT(emu::disassembleDataProcessing(0x4081FFA6) == "addmi r15, r1, r6, lsr #0x1f");
        EXPECT(emu::disassembleDataProcessing(0x50A93C30) == "adcpl r3, r9, r0, lsr r12");
        EXPECT(emu::disassembleDataProcessing(0x60C1B0C3) == "sbcvs r11, r1, r3, asr #0x1");
        EXPECT(emu::disassembleDataProcessing(0x70E73250) == "rscvc r3, r7, r0, asr r2");
        EXPECT(emu::disassembleDataProcessing(0x81190263) == "tsthi r9, r3, ror #0x4");
        EXPECT(emu::disassembleDataProcessing(0x913A067F) == "teqls r10, r15, ror r6");
        EXPECT(emu::disassembleDataProcessing(0xA15A0069) == "cmpge r10, r9, rrx");
        EXPECT(emu::disassembleDataProcessing(0xB3720049) == "cmnlt r2, #0x49");
        EXPECT(emu::disassembleDataProcessing(0xC1821000) == "orrgt r1, r2, r0");
        EXPECT(emu::disassembleDataProcessing(0xD1B06001) == "movles r6, r1");
        EXPECT(emu::disassembleDataProcessing(0xE1C74002) == "bic r4, r7, r2");
        EXPECT(emu::disassembleDataProcessing(0xF1E0D003) == "mvnnv r13, r3");
    },

    CASE("Multiply Instructions") {
        EXPECT(emu::disassembleMultiply(0x10110895) == "mulnes r1, r5, r8");
        EXPECT(emu::disassembleMultiply(0xE0090B9F) == "mul r9, r15, r11");
        EXPECT(emu::disassembleMultiply(0xE0304A91) == "mlas r0, r1, r10, r4");
        EXPECT(emu::disassembleMultiply(0x4023629B) == "mlami r3, r11, r2, r6");
    },

    CASE("Multiply Long Instructions") {
        EXPECT(emu::disassembleMultiplyLong(0xE0834192) == "umull r4, r3, r2, r1");
        EXPECT(emu::disassembleMultiplyLong(0x309BA290) == "umullccs r10, r11, r0, r2");
        EXPECT(emu::disassembleMultiplyLong(0xE0B0F793) == "umlals r15, r0, r3, r7");
        EXPECT(emu::disassembleMultiplyLong(0x00A5469A) == "umlaleq r4, r5, r10, r6");
        EXPECT(emu::disassembleMultiplyLong(0xE0D10D9C) == "smulls r0, r1, r12, r13");
        EXPECT(emu::disassembleMultiplyLong(0x70CCD091) == "smullvc r13, r12, r1, r0");
        EXPECT(emu::disassembleMultiplyLong(0xE0E25E94) == "smlal r5, r2, r4, r14");
        EXPECT(emu::disassembleMultiplyLong(0x10FCAF9D) == "smlalnes r10, r12, r13, r15");
    },

    CASE("Single Data Swap Instructions") {
        EXPECT(emu::disassembleDataSwap(0xE1024095) == "swp r4, r5, [r2]");
        EXPECT(emu::disassembleDataSwap(0x610FA092) == "swpvs r10, r2, [r15]");
        EXPECT(emu::disassembleDataSwap(0xC14C3099) == "swpgtb r3, r9, [r12]");
        EXPECT(emu::disassembleDataSwap(0xE1401098) == "swpb r1, r8, [r0]");
    },

    CASE("Branch and Exchange Instructions") {
        EXPECT(emu::disassembleBranchExchange(0xE12FFF11) == "bx r1");
        EXPECT(emu::disassembleBranchExchange(0xB12FFF1C) == "bxlt r12");
    },

    CASE("Halfword Data Transfer Instructions") {
        EXPECT(emu::disassembleHalfwordTransfer(0xE1D255B2) == "ldrh r5, [r2, #+0x52]");
        EXPECT(emu::disassembleHalfwordTransfer(0xE18A00B4) == "strh r0, [r10, +r4]");
        EXPECT(emu::disassembleHalfwordTransfer(0xE1787FDF) == "ldrsb r7, [r8, #-0xff]!");
        EXPECT(emu::disassembleHalfwordTransfer(0xE13940FF) == "ldrsh r4, [r9, -r15]!");
        EXPECT(emu::disassembleHalfwordTransfer(0x0050E0B1) == "ldreqh r14, [r0], #-0x1");
        EXPECT(emu::disassembleHalfwordTransfer(0xA08BD0B2) == "strgeh r13, [r11], +r2");
        EXPECT(emu::disassembleHalfwordTransfer(0xD15554DF) == "ldrlesb r5, [r5, #-0x4f]");
        EXPECT(emu::disassembleHalfwordTransfer(0x91B230FF) == "ldrlssh r3, [r2, +r15]!");
    },

    CASE("Single Data Transfer Instructions") {
        
    }
};