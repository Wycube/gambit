#include "core/cpu/arm/Arm.hpp"
#include "common/Utilities.hpp"
#include "common/Version.hpp"

#include <iostream>
#include <bitset>
#include <random>
#include <chrono>
#include <cstring>


void print_instr(u32 instruction) {
    std::cout << "Instruction: " << std::bitset<32>(instruction) << "\n";
    emu::ArmInstruction parsed = emu::parseInstruction(instruction);

    std::cout << "Dissasembly: " << parsed.dissasembly << "\n";
}

int main() {
    std::mt19937 rand;
    std::uniform_int_distribution<u32> dist(0, std::numeric_limits<u32>::max());

    // for(int i = 0; i < 1000; i++) {
    //     //std::cout << std::bitset<32>(dist(rand)) << "\n";
    //     u32 rand_instr = dist(rand);

    //     print_instr(rand_instr);
    // }

    printf("Version %s.%s-%s\n", common::VERSION_MAJOR, common::VERSION_MINOR, common::GIT_DESC);

    //Test every data processing opcode
    // for(u8 i = 0; i < 16; i++) {
    //     u32 instr = 0b1110'000'0000'1'0000'0001'0000'0'11'1'0100;
    //     instr |= i << 21;
    
    //     print_instr(instr);
    // }

    // u32 instr = 0b11101111001100000100011011010101;

    // print_instr(instr);
}