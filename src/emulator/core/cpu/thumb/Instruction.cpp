#include "Instruction.hpp"
#include "Disassembly.hpp"
#include "common/Pattern.hpp"


namespace emu {

/*
 * Only 8 bits are needed to decode a 16-bit THUMB instruction: bits 8-15 (8).
 */
constexpr char THUMB_ENCODINGS[20][9] = {
    "000>>xxx", //Move Shifted Register
    "00011xxx", //Add/Subtract Register (Register and Immediate)
    "001xxxxx", //Add/Subtract/Compare/Move Immediate
    "010000xx", //ALU Operation
    "010001>>", //Hi Register Operation
    "01000111", //Branch and Exchange
    "01001xxx", //PC-Relative Load
    "0101xx0x", //Load/Store Register Offset
    "0101xx1x", //Load/Store Sign-Extended Byte/Halfword
    "011xxxxx", //Load/Store Immediate Offset
    "1000xxxx", //Load/Store Halfword
    "1001xxxx", //SP-Relative Load/Store
    "1010xxxx", //Load Address
    "10110000", //Adjust Stack Pointer
    "1011x10x", //Push/Pop Registers
    "1100xxxx", //Load/Store Multiple
    "1101>>>x", //Conditional Branch
    "11011111", //Software Interrupt
    "11100xxx", //Unconditional Branch
    "1111xxxx"  //Long Branch with Link
};

auto thumbDetermineType(u16 instruction) -> ThumbInstructionType {
    const u8 decoding_bits = instruction >> 8;

    return static_cast<ThumbInstructionType>(common::const_match_bits<20, 9, THUMB_ENCODINGS>(decoding_bits, THUMB_UNDEFINED));
}

auto thumbDecodeInstruction(u16 instruction, u32 address, u16 prev) -> ThumbInstruction {
    ThumbInstructionType type = thumbDetermineType(instruction);
    std::string disassembly = thumbDisassemblyFuncs[type](instruction, address, prev);

    return ThumbInstruction{instruction, type, disassembly};
}

} //namespace emu