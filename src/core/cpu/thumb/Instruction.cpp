#include "Instruction.hpp"
#include "common/Pattern.hpp"


namespace emu {

/*
 * Only 8 bits are needed to decode a 16-bit THUMB instruction: bits 15-8 (8).
 */
const char *THUMB_ENCODINGS[] = {
    "000>>xxx", //Move Shifted Register
    "00011xxx", //Add/Subtract Register (Register and Immediate)
    "001xxxxx", //Add/Subtract/Compare/Move Immediate
    "010000xx", //Data-Processing Register
    "010001>>", //Hi Register Operations
    "01000111", //Branch/Exchange
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
    "1101>>>x", //Condition Branch
    "11011111", //Software Interrupt
    "11100xxx", //Unconditional Branch
    "1111xxxx"  //Long Branch with Link
};

auto thumbDetermineType(u16 instruction) -> ThumbInstructionType {
    u8 decoding_bits = instruction >> 8;

    return static_cast<ThumbInstructionType>(common::match_bits(decoding_bits, THUMB_ENCODINGS));
}

auto thumbDecodeInstruction(u16 instruction) -> ThumbInstruction {

}

} //namespace emu