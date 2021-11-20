#include "CPU.hpp"

#include <vector>


namespace emu {

enum ConditionCode : u8 {
    EQ = 0b0000, //Equal
    NE = 0b0001, //Not Equal
    CS = 0b0010, //Carry Set
    CC = 0b0011, //Carry Clear
    MI = 0b0100, //Negative / Minus
    PL = 0b0101, //Positive / Plus
    VS = 0b0110, //Overflow
    VC = 0b0111, //No Overflow
    HI = 0b1000, //Unsigned Higher
    LS = 0b1001, //Unsigned Lower or Same
    GE = 0b1010, //Signed Greater than or Equal
    LT = 0b1011, //Signed Less than
    GT = 0b1100, //Signed Greater than
    LE = 0b1101, //Signed Less than or Equal
    AL = 0b1110, //Always (unconditional)
    UNPREDICTABLE = 0b1111 //Unpredictable on ARMv4
};

/* 
 * x means 1 or 0
 * < means can't all be unset
 * i.e. << can be 01, 10, 11, but not 00
 * Only 12 bits are needed to decode a 32-bit ARM instruction
 * bits 27-20 (8) + bits 7-4 (4)
 */
static std::vector<std::string> encodings_arm = {
    //Data Processing
    "000xxxxxxxx0", //Immediate Shift
    "000xxxxx0xx1", //Register Shift
    "001xxxxxxxxx", //Immediate
    
    //Multiply
    "000000xx1001", //Multiply
    "00001xxx1001", //Multiply Long

    "00010x001001", //Single Data Swap
    "000100100001", //Branch and Exchange

    //Halfword Data Transfer
    "000xx0xx1<<1", //Register Offset
    "000xx1xx1<<1", //Immediate Offset

    //Single Data Transfer
    "010xxxxxxxxx", //Immediate Offset
    "011xxxxxxxx0", //Register Offset

    "011xxxxxxxx1", //Undefined
    "100xxxxxxxxx", //Block Data Transfer
    "101xxxxxxxxx", //Branch
    "110xxxxxxxxx", //Coprocessor Data Transfer
    "1110xxxxxxx0", //Coprocessor Data Operation
    "1110xxxxxxx1", //Coprocessor Register Transfer
    "1111xxxxxxxx"  //Software Interrupt
};

static std::string instruction_types[18] = {
    "Data Processing Immediate Shift",
    "Data Processing Register Shift",
    "Data Processing Immediate",
    "Multiply",
    "Multiply Long",
    "Single Data Swap",
    "Branch and Exchange",
    "Halfword Data Transfer: Register Offset",
    "Halfword Data Transfer: Immediate Offset",
    "Single Data Transfer Immediate Offset",
    "Single Data Transfer Register Offset",
    "Undefined",
    "Block Data Transfer",
    "Branch",
    "Coprocessor Data Transfer",
    "Coprocessor Data Operation",
    "Coprocessor Register Transfer",
    "Software Interrupt"
};

auto determineType(u32 instruction) -> std::string {
    u16 decoding_bits = (((instruction >> 16) & 0xFF0) | ((instruction >> 4) & 0xF));

    for(int i = 0; i < encodings_arm.size(); i++) {
        std::string encoding = encodings_arm[i];

        bool match = true;
        for(int j = 0; j < 12; j++) {
            u8 instr_bit = (decoding_bits >> (11 - j)) & 1;
            u8 encode_char = encoding[j];

            if(encode_char == '0') {
                if(instr_bit == 0) {
                    continue;
                } else {
                    match = false;
                    break;
                }
            } else if(encode_char == '1') {
                if(instr_bit == 1) {
                    continue;
                } else {
                    match = false;
                    break;
                }
            } else if(encode_char == 'x') {
                continue;
            } else if(encode_char == '>') {
                j++;
                u8 next_bit = (decoding_bits >> (11 - j)) & 1;

                if((instr_bit == 0) && (next_bit == 0)) {
                    match = false;
                    break;
                }
            }
        }

        if(match) {
            return instruction_types[i];
        }
    }

    return "Unable to detect type";
}

auto decodeInstructionARM(u32 instruction) -> Decoded {
    //Every instruction in ARMv4 can be executed conditionally,
    //and as such start with a 4-bit condition field
    u8 condition = instruction >> 28;
    std::string conds[16] = 
    {"Equal", "Not Equal", "Carry Set", "Carry Clear", "Negative", "Positive", "Overflow", "No Overflow", "Unsigned Higher",
     "Unsigned Lower or Same", "Signed Greater than or Equal", "Signed Less than", "Signed Greater than", "Signed Less than or Equal", "Always", "Unpredictable"};
    
    //This next 3-bit sequence will determine how it is decoded next
    u8 next_bits = (instruction >> 25) & 0b111;

    //Decode based on these bits
    std::string type;
    
    return {condition, next_bits, type, conds[condition]};
}

auto decodeInstructionTHUMB(u16 instruction) -> Decoded {
    return {0, 0, "THUMB"};
}

} //namespace emu