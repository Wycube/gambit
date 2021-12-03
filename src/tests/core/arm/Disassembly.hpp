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
        EXPECT(emu::disassembleSingleTransfer(0xE595070F) == "ldr r0, [r5, #+0x70f]");
        EXPECT(emu::disassembleSingleTransfer(0x1751200C) == "ldrneb r2, [r1, -r12]");
        EXPECT(emu::disassembleSingleTransfer(0x07993D0F) == "ldreq r3, [r9, +r15, lsl #0x1a]");
        EXPECT(emu::disassembleSingleTransfer(0xE7093060) == "str r3, [r9, -r0, rrx]");
        EXPECT(emu::disassembleSingleTransfer(0x7524AF2F) == "strvc r10, [r4, #-0xf2f]!");
        EXPECT(emu::disassembleSingleTransfer(0x37E7400D) == "strccb r4, [r7, +r13]!");
        EXPECT(emu::disassembleSingleTransfer(0xE760B621) == "strb r11, [r0, -r1, lsr #0xc]!");
        EXPECT(emu::disassembleSingleTransfer(0xC4B359C7) == "ldrgtt r5, [r3], #+0x9c7");
        EXPECT(emu::disassembleSingleTransfer(0xE6B0900B) == "ldrt r9, [r0], +r11");
        EXPECT(emu::disassembleSingleTransfer(0x86242106) == "strhit r2, [r4], -r6, lsl #0x2");
        EXPECT(emu::disassembleSingleTransfer(0xE62517A7) == "strt r1, [r5], -r7, lsr #0xf");
        EXPECT(emu::disassembleSingleTransfer(0xA6649F41) == "strgebt r9, [r4], -r1, asr #0x1e");
        EXPECT(emu::disassembleSingleTransfer(0xE66F03E8) == "strbt r0, [r15], -r8, ror #0x7");
        EXPECT(emu::disassembleSingleTransfer(0xE6F10062) == "ldrbt r0, [r1], +r2, rrx");
    },

    CASE("Undefined Instructions") {
        EXPECT(emu::disassembleUndefined(0x06000010) == "undefined");
        EXPECT(emu::disassembleUndefined(0x66A38F9C) == "undefined");
    },

    CASE("Block Data Transfer Instructions") {
        EXPECT(emu::disassembleBlockTransfer(0xE89502A8) == "ldmia r5, {r3, r5, r7, r9}");
        EXPECT(emu::disassembleBlockTransfer(0xE9A29020) == "stmib r2!, {r5, r12, r15}");
        EXPECT(emu::disassembleBlockTransfer(0xE87AB007) == "ldmda r10!, {r0, r1, r2, r12, r13, r15}^");
        EXPECT(emu::disassembleBlockTransfer(0xE9404200) == "stmdb r0, {r9, r14}^");
    },

    CASE("Branch Instructions") {
        EXPECT(emu::disassembleBranch(0xEA1570AA) == "b #0x55c2b0");
        EXPECT(emu::disassembleBranch(0x6A05BBB1) == "bvs #0x16eecc");
        EXPECT(emu::disassembleBranch(0x0B123FEE) == "bleq #0x48ffc0");
        EXPECT(emu::disassembleBranch(0xEB3DFFA3) == "bl #0xf7fe94");
    },

    //Coprocessor Instruction tests would go here, except they're not even used on GBA AFAIK

    CASE("Software Interrupt Instructions") {
        EXPECT(emu::disassembleSoftwareInterrupt(0xEF000041) == "swi #65");
        EXPECT(emu::disassembleSoftwareInterrupt(0x1F07BFAD) == "swine #507821");
    }
};