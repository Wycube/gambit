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
    }
};