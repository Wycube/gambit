#pragma once

#include "common/Pattern.hpp"

#include <lest/lest.hpp>
#include <string>


const char *constant_patterns_bit[] = {
    "0",
    "1"
};

const char *constant_patterns[] = {
    "10101",
    "00000000",
    "11111111",
    "00000001",
    "11001100110011001100"
};


const char *variable_patterns[] = {
    "1010x",
    "x000x",
    "11xxxx11",
    "10x10x10x10x10x",
    "xxxxxxxxx"
};

const char *exclusion_patterns[] = {
    "00000<",
    "0100>1",
    "00>>11",
    "100<><",
    "1<1><1>"
};


const lest::test common_pattern_tests[] = {
    CASE("Constant Patterns") {
        EXPECT(common::match_bits<u8>(0b11110, constant_patterns_bit) == 0);
        EXPECT(common::match_bits<u8>(0b00001, constant_patterns_bit) == 1);
        EXPECT(common::match_bits<u8>(0b10101, constant_patterns) == 0);
        EXPECT(common::match_bits<u8>(0b00000000, constant_patterns) == 1);
        EXPECT(common::match_bits<u8>(0b11111111, constant_patterns) == 2);
        EXPECT(common::match_bits<u8>(0b00000001, constant_patterns) == 3);
        EXPECT(common::match_bits<u8>(0b10000001, constant_patterns) == 5);
        EXPECT(common::match_bits<u32>(0b1001100110011001100, constant_patterns) == 5);
        EXPECT(common::match_bits<u32>(0b11001100110011001100, constant_patterns) == 4);
    },

    CASE("Variable Patterns") {
        EXPECT(common::match_bits<u8>(0b10100, variable_patterns) == 0);
        EXPECT(common::match_bits<u8>(0b10101, variable_patterns) == 0);
        EXPECT(common::match_bits<u8>(0b00000, variable_patterns) == 1);
        EXPECT(common::match_bits<u8>(0b00001, variable_patterns) == 1);
        EXPECT(common::match_bits<u8>(0b10000, variable_patterns) == 1);
        EXPECT(common::match_bits<u8>(0b10001, variable_patterns) == 1);
        EXPECT(common::match_bits<u8>(0b11000011, variable_patterns) == 2);
        EXPECT(common::match_bits<u8>(0b11111111, variable_patterns) == 2);
        EXPECT(common::match_bits<u16>(0b100101101100100, variable_patterns) == 3);
        EXPECT(common::match_bits<u16>(0b101100101101101, variable_patterns) == 3);
        EXPECT(common::match_bits<u8>(0b00111100, variable_patterns) == 5);
        EXPECT(common::match_bits<u8>(0b01111101, variable_patterns) == 5);
        EXPECT(common::match_bits<u16>(0b01111101, variable_patterns) == 4);
        EXPECT(common::match_bits<u16>(0b0110110000, variable_patterns) == 1);
    },

    CASE("Exclusion Patterns") {
        EXPECT(common::match_bits<u8>(0b000001, exclusion_patterns) == 0);
        EXPECT(common::match_bits<u8>(0b000000, exclusion_patterns) == 5);
        EXPECT(common::match_bits<u8>(0b010011, exclusion_patterns) == 5);
        EXPECT(common::match_bits<u8>(0b010001, exclusion_patterns) == 1);
        EXPECT(common::match_bits<u8>(0b000011, exclusion_patterns) == 2);
        EXPECT(common::match_bits<u8>(0b000111, exclusion_patterns) == 2);
        EXPECT(common::match_bits<u8>(0b001011, exclusion_patterns) == 2);
        EXPECT(common::match_bits<u8>(0b001111, exclusion_patterns) == 5);

        for(int i = 0; i < 8; i++) {
            EXPECT(common::match_bits<u8>(0b100000 | i, exclusion_patterns) == (i == 0b010 ? 5 : 3));
        }

        for(int i = 0; i < 16; i++) {
            u8 value = 0b1010010 | ((i << 2) & 0x20) | ((i << 1) & 0x8) | ((i << 1) & 0x4) | (i & 0x1);
            EXPECT(common::match_bits<u8>(value, exclusion_patterns) == (i == 0b0101 ? 5 : 4));
        }
    }
};