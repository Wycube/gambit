#pragma once

#include <string>


namespace emu {

const std::string THUMB_ENCODINGS[] = {
    "000>>xxx", //Move Shifted Register

    //Add/Subtract Register
    "000110xx", //Register
    "000111xx", //Immediate

    "001xxxxx", //Add/Subtract/Compare/Move Immediate
    "010000xx", //Data-Processing Register
    "010001>>", //Hi Register Operations
    "01000111", //Branch/Exchange
    "01001xxx", //PC-Relative Load
    "0101xxxx", //Load/Store Register Offset
    "011xxxxx", //Load/Store Word/Byte Immediate Offset
    "1000xxxx", //Load/Store Halfword Immediate Offset
    "1001xxxx", //SP-Relative Load/Store
    "1010xxxx", //Load Address
    "10110000", //Adjust Stack Pointer
    "1011x10x", //Push/Pop Registers
    "1100xxxx", //Load/Store Multiple
    "1101<<<x", //Condition Branch
    "11011111", //Software Interrupt
    "11100xxx", //Unconditional Branch
    "1111xxxx"  //Long Branch with Link
};

} //namespace emu