#include "Arm.hpp"
#include "Disassembly.hpp"

#include <bitset>


namespace emu {

static InstructionType types[sizeof(ARM_ENCODINGS)] = {
    BRANCH_AND_EXCHANGE,
    DATA_PROCESSING, DATA_PROCESSING, DATA_PROCESSING,
    MULTIPLY, 
    MULTIPLY_LONG, 
    SINGLE_DATA_SWAP, 
    HALFWORD_DATA_TRANSFER, 
    SINGLE_DATA_TRANSFER, SINGLE_DATA_TRANSFER, 
    UNDEFINED,
    BLOCK_DATA_TRANSFER,
    BRANCH,
    COPROCESSOR_DATA_TRANSFER,
    COPROCESSOR_DATA_OPERATION,
    COPROCESSOR_REGISTER_TRANSFER,
    SOFTWARE_INTERRUPT
};

auto determineType(u32 instruction) -> InstructionType {
    u16 decoding_bits = (((instruction >> 16) & 0xFF0) | ((instruction >> 4) & 0xF));

    for(int i = 0; i < sizeof(ARM_ENCODINGS); i++) {
        std::string encoding = ARM_ENCODINGS[i];

        bool match = true;
        for(int j = 11; j >= 0; j--) {
            u8 instr_bit = (decoding_bits >> j) & 1;
            u8 encode_char = encoding[11 - j];

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
                j--;
                u8 next_bit = (decoding_bits >> j) & 1;

                if((instr_bit == 0) && (next_bit == 0)) {
                    match = false;
                    break;
                }
            }
        }

        //printf("Encoding: %s | Match: %s | Decoding Bits: %s\n", encoding.c_str(), match ? "true" : "false", std::bitset<12>(decoding_bits).to_string().c_str());

        if(match) {
            return types[i];
        }
    }

    return UNKNOWN;
}

auto parseInstruction(u32 instruction) -> ArmInstruction {
    InstructionType type = determineType(instruction);
    ConditionCode condition = static_cast<ConditionCode>(instruction >> 28);
    std::string dissasembly;

    switch(type) {
        case DATA_PROCESSING : dissasembly = disassembleDataProcessing(instruction);
        break;
        case MULTIPLY : dissasembly = disassembleMultiply(instruction);
        break;
        case MULTIPLY_LONG : dissasembly = disassembleMultiplyLong(instruction);
        break;
        case SINGLE_DATA_SWAP : dissasembly = disassembleDataSwap(instruction);
        break;
        case BRANCH_AND_EXCHANGE : dissasembly = disassembleBranchExchange(instruction);
        break;
        case HALFWORD_DATA_TRANSFER : dissasembly = disassembleHalfwordTransfer(instruction);
        break;
        case SINGLE_DATA_TRANSFER : dissasembly = disassembleSingleTransfer(instruction);
        break;
        case UNDEFINED : dissasembly = disassembleUndefined(instruction);
        break;
        case BLOCK_DATA_TRANSFER : dissasembly = disassembleBlockTransfer(instruction);
        break;
        case BRANCH : dissasembly = disassembleBranch(instruction);
        break;
        case COPROCESSOR_DATA_TRANSFER : dissasembly = disassembleCoDataTransfer(instruction);
        break;
        case COPROCESSOR_DATA_OPERATION : dissasembly = disassembleCoDataOperation(instruction);
        break;
        case COPROCESSOR_REGISTER_TRANSFER : dissasembly = disassembleCoRegisterTransfer(instruction);
        break;
        case SOFTWARE_INTERRUPT : dissasembly = disassembleSoftwareInterrupt(instruction);
        break;
        default: dissasembly = "Unknown";
    }

    return ArmInstruction{instruction, dissasembly, condition, type};
}

} //namespace emu