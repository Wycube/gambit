#pragma once

#include "tests/core/arm/DisassemblyTests.hpp"
#include "tests/core/arm/DecodeTests.hpp"
#include "tests/core/thumb/DisassemblyTests.hpp"
#include "tests/common/PatternTests.hpp"

#define TEST_VEC(specification) lest::tests(specification, specification + sizeof(specification) / sizeof(specification[0]))


const std::vector<lest::tests> all_tests = {
    TEST_VEC(arm_disassembly_tests),
    TEST_VEC(arm_decode_tests),
    TEST_VEC(thumb_disassembly_tests),
    TEST_VEC(common_pattern_tests)
};