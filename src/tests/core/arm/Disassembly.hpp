#pragma once

#include "core/cpu/arm/Disassembly.hpp"

#include <lest/lest.hpp>
#include <string>


const lest::test arm_disassembly_tests[] = {
    CASE("Data Processing Instructions") {
        EXPECT(emu::disassembleDataProcessing(0xE0000001) == "and r0, r0, r1");
    }
};