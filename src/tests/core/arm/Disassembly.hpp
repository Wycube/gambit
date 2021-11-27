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
    }
};