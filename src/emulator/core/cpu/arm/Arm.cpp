#include "Arm.hpp"
#include "Dissasembly.hpp"

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
        case DATA_PROCESSING : dissasembly = dissasembleDataProcessing(instruction);
        break;
        case MULTIPLY : dissasembly = dissasembleMultiply(instruction);
        break;
        case MULTIPLY_LONG : dissasembly = dissasembleMultiplyLong(instruction);
        break;
        case SINGLE_DATA_SWAP : dissasembly = dissasembleDataSwap(instruction);
        break;
        case BRANCH_AND_EXCHANGE : dissasembly = dissasembleBranchExchange(instruction);
        break;
        case HALFWORD_DATA_TRANSFER : dissasembly = dissasembleHalfwordTransfer(instruction);
        break;
        case SINGLE_DATA_TRANSFER : dissasembly = dissasembleSingleTransfer(instruction);
        break;
        case UNDEFINED : dissasembly = dissasembleUndefined(instruction);
        break;
        case BLOCK_DATA_TRANSFER : dissasembly = dissasembleBlockTransfer(instruction);
        break;
        case BRANCH : dissasembly = dissasembleBranch(instruction);
        break;
        case COPROCESSOR_DATA_TRANSFER : dissasembly = dissasembleCoDataTransfer(instruction);
        break;
        case COPROCESSOR_DATA_OPERATION : dissasembly = dissasembleCoDataOperation(instruction);
        break;
        case COPROCESSOR_REGISTER_TRANSFER : dissasembly = dissasembleCoRegisterTransfer(instruction);
        break;
        case SOFTWARE_INTERRUPT : dissasembly = dissasembleSoftwareInterrupt(instruction);
        break;
        default: dissasembly = "Unknown";
    }

    return ArmInstruction{instruction, dissasembly, condition, type};
}

} //namespace emu