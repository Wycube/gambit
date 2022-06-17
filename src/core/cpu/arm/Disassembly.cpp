#include "Disassembly.hpp"
#include "common/StringUtils.hpp"
#include "common/Bits.hpp"

#include <cassert>


//TODO:
// - Use a formatting library like {fmt}
// - Use something better than an assert to handle errors

namespace emu {

std::string (*armDisassemblyFuncs[16])(u32, u32) = {
    armDisassembleBranchExchange,
    armDisassemblePSRTransfer,
    armDisassembleDataProcessing,
    armDisassembleMultiply,
    armDisassembleMultiplyLong,
    armDisassembleDataSwap,
    armDisassembleHalfwordTransfer,
    armDisassembleSingleTransfer,
    armDisassembleUndefined,
    armDisassembleBlockTransfer,
    armDisassembleBranch,
    armDisassembleCoDataTransfer,
    armDisassembleCoDataOperation,
    armDisassembleCoRegisterTransfer,
    armDisassembleSoftwareInterrupt,
    armDisassembleInvalid
};


//0b1111 is obselete on armv4 and is unpredictable
//but I'll still use NV mnemonic extension to identify them.
static const char *CONDITION_EXTENSIONS[16] = {
    "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
    "hi", "ls", "ge", "lt", "gt", "le", "",   "nv"
};

static const char *SHIFT_MNEMONICS[4] = {
    "lsl", "lsr", "asr", "ror"
};


//BX{<cond>} <Rm>
auto armDisassembleBranchExchange(u32 instruction, u32 address) -> std::string {
    u8 condition = instruction >> 28;
    u8 rm = instruction & 0xF;

    std::string disassembly = "bx";
    disassembly += CONDITION_EXTENSIONS[condition];
    disassembly += " r" + std::to_string(rm);

    return disassembly;
}


//MRS{<cond>} <Rd>, CPSR|SPSR
//MSR{<cond>} (CPSR|SPSR)_<fields>, #<immediate>
//MSR{<cond>} (CPSR|SPSR)_<fields>, <Rm>
auto armDisassemblePSRTransfer(u32 instruction, u32 address) -> std::string {
    u8 condition = instruction >> 28;
    bool i = (instruction >> 25) & 0x1;
    bool r = (instruction >> 22) & 0x1;
    bool s = (instruction >> 21) & 0x1;
    u8 fields = (instruction >> 16) & 0xF;
    u8 rd = (instruction >> 12) & 0xF;

    std::string disassembly;

    disassembly = s ? "msr" : "mrs";
    disassembly += CONDITION_EXTENSIONS[condition];

    if(s) {
        u8 shift_imm = (instruction >> 8) & 0xF;
        u32 immediate = bits::ror(instruction & 0xFF, shift_imm << 1);

        disassembly += r ? " spsr_" : " cpsr_";
        disassembly += fields & 0x1 ? "c" : "";
        disassembly += (fields >> 1) & 0x1 ? "x" : "";
        disassembly += (fields >> 2) & 0x1 ? "s" : "";
        disassembly += (fields >> 3) & 0x1 ? "f" : "";
        disassembly += ", ";
        disassembly += i ? "#0x" + common::hex(immediate) : "r" + std::to_string(instruction & 0xF);
    } else {
        disassembly += " r" + std::to_string(rd);
        disassembly += ", "; 
        disassembly += r ? "spsr" : "cpsr";
    }

    return disassembly;
}


//<Rm>, <shift> <Rs>
auto registerShift(u16 shift_operand) -> std::string {
    u8 rs = shift_operand >> 8;
    u8 shift = (shift_operand >> 5) & 0x3;
    u8 rm = shift_operand & 0xF;

    return "r" + std::to_string(rm) + ", " + SHIFT_MNEMONICS[shift] + " r" + std::to_string(rs);
}

//<Rm>
//<Rm>, RRX
//<Rm>, <shift> #<shift_imm>
auto immediateShift(u16 shift_operand) -> std::string {
    u8 shift_imm = shift_operand >> 7;
    u8 shift = (shift_operand >> 5) & 0x3;
    u8 rm = shift_operand & 0xF;

    if(shift_imm == 0) {
        if(shift == 0) {
            //Special case, just use register Rm
            return "r" + std::to_string(rm);
        } else if(shift == 3) {
            //Special case, RRX (Rotate Right with Extend)
            return "r" + std::to_string(rm) + ", rrx";
        }
    }

    return "r" + std::to_string(rm) + ", " + SHIFT_MNEMONICS[shift] + " #0x" + common::hex(shift_imm);
}

//#<immediate>
auto immediate(u16 shift_operand) -> std::string {
    u8 rotate_imm = ((shift_operand >> 8) & 0xF) * 2; //Shifts an even number of bits
    u8 immed_8 = shift_operand & 0xFF;
    u32 immediate = (immed_8 >> rotate_imm) | (immed_8 << (32 - rotate_imm)); //Rotate right by (rotate_imm * 2)

    return "#0x" + common::hex(immediate);
}

//AND{<cond>}{S} <Rd>, <Rn>, <shift_operand> - Math
//CMP{<cond>}{S} <Rn>, <shift_operand>       - Compare
//MOV{<cond>}{S} <Rd>, <shift_operand>       - Move
auto armDisassembleDataProcessing(u32 instruction, u32 address) -> std::string {
    static const char *OPCODE_MNEMONICS[16] = {
        "and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc",
        "tst", "teq", "cmp", "cmn", "orr", "mov", "bic", "mvn"
    };

    u8 condition = instruction >> 28;
    u8 opcode = (instruction >> 21) & 0xF;
    u8 rn = (instruction >> 16) & 0xF;
    u8 rd = (instruction >> 12) & 0xF;

    std::string disassembly;
    disassembly += OPCODE_MNEMONICS[opcode]; 
    disassembly += CONDITION_EXTENSIONS[condition];

    if(opcode < 8 || opcode == 12 || opcode == 14) {
        //<Rd>, <Rn>
        disassembly += (instruction >> 20) & 0x1 ? "s" : ""; //Save to CPSR bit
        disassembly += " r" + std::to_string(rd);
        disassembly += ", r" + std::to_string(rn);
    } else if(opcode > 7 && opcode < 12) {
        //<Rn>
        //Doesn't use the S-bit, should be one
        disassembly += " r" + std::to_string(rn);
    } else if(opcode == 13 || opcode == 15) {
        //<Rd>
        disassembly += (instruction >> 20) & 0x1 ? "s" : ""; //Save to CPSR bit
        disassembly += " r" + std::to_string(rd);
    }

    //Check the I (immediate) bit and the 4th bit, that determines immediate shift or register shift.
    u8 addressing_mode = (instruction >> 25) & 0x1 ? 0 : (instruction >> 4) & 0x1 ? 2 : 1;

    //Shift operand is first 12 bits
    switch(addressing_mode) {
        case 0 : disassembly += ", " + immediate(instruction & 0xFFF);
        break;
        case 1 : disassembly += ", " + immediateShift(instruction & 0xFFF);
        break;
        case 2 : disassembly += ", " + registerShift(instruction & 0xFFF);
        break;
    }

    return disassembly;
}


//MUL{<cond>}{S} <Rd>, <Rm>, <Rs>
//MLA{<cond>}{S} <Rd>, <Rm>, <Rs>, <Rn>
auto armDisassembleMultiply(u32 instruction, u32 address) -> std::string {
    u8 condition = instruction >> 28;
    u8 rd = (instruction >> 16) & 0xF;
    u8 rn = (instruction >> 12) & 0xF;
    u8 rs = (instruction >> 8) & 0xF;
    u8 rm = instruction & 0xF;
    bool accumulate = (instruction >> 21) & 0x1;

    std::string disassembly;
    disassembly += accumulate ? "mla" : "mul"; //Accumulate bit
    disassembly += CONDITION_EXTENSIONS[condition];
    disassembly += (instruction >> 20) & 0x1 ? "s" : ""; //Save to CPSR bit
    disassembly += " r" + std::to_string(rd);
    disassembly += ", r" + std::to_string(rm);
    disassembly += ", r" + std::to_string(rs);
    disassembly += accumulate ? ", r" + std::to_string(rn) : "";

    return disassembly;
}

//UMULL{<cond>}{S} <RdLo>, <RdHi>, <Rm>, <Rs>
//UMLAL{<cond>}{S} <RdLo>, <RdHi>, <Rm>, <Rs>
//SMULL{<cond>}{S} <RdLo>, <RdHi>, <Rm>, <Rs>
//SMLAL{<cond>}{S} <RdLo>, <RdHi>, <Rm>, <Rs>
auto armDisassembleMultiplyLong(u32 instruction, u32 address) -> std::string {
    u8 condition = instruction >> 28;
    u8 rd_hi = (instruction >> 16) & 0xF;
    u8 rd_lo = (instruction >> 12) & 0xF;
    u8 rs = (instruction >> 8) & 0xF;
    u8 rm = instruction & 0xF;

    std::string disassembly;
    disassembly += (instruction >> 22) & 0x1 ? "s" : "u"; //Unsigned bit
    disassembly += (instruction >> 21) & 0x1 ? "mlal" : "mull"; //Accumulate bit
    disassembly += CONDITION_EXTENSIONS[condition];
    disassembly += (instruction >> 20) & 0x1 ? "s" : ""; //Save to CPSR bit
    disassembly += " r" + std::to_string(rd_lo);
    disassembly += ", r" + std::to_string(rd_hi);
    disassembly += ", r" + std::to_string(rm);
    disassembly += ", r" + std::to_string(rs);

    return disassembly;
}


//SWP{<cond>}{B} <Rd>, <Rm>, [Rn]
auto armDisassembleDataSwap(u32 instruction, u32 address) -> std::string {
    u8 condition = instruction >> 28;
    u8 rn = (instruction >> 16) & 0xF;
    u8 rd = (instruction >> 12) & 0xF;
    u8 rm = instruction & 0xF;

    std::string disassembly = "swp";
    disassembly += CONDITION_EXTENSIONS[condition];
    disassembly += (instruction >> 22) & 0x1 ? "b" : ""; //Byte bit
    disassembly += " r" + std::to_string(rd);
    disassembly += ", r" + std::to_string(rm);
    disassembly += ", [r" + std::to_string(rn) + "]";

    return disassembly;
}


//[<Rn>, +/-<Rm>]
//[<Rn>, +/-<Rm>]!  Pre-indexed
//[<Rn>], +/-<Rm>   Post-indexed
auto registerOffset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string {
    u8 rm = addr_mode & 0xF;
    std::string sign = u ? "+" : "-";

    std::string disassembly = "[r" + std::to_string(rn);

    if(p) {
        disassembly += ", " + sign + "r" + std::to_string(rm) + "]";
        disassembly += w ? "!" : "";
    } else {
        disassembly += "], " + sign + "r" + std::to_string(rm);
    }

    return disassembly;
}

//[<Rn>, #+/-<offset_8>]
//[<Rn>, #+/-<offset_8>]!  Pre-indexed
//[<Rn>], #+/-<offset_8>   Post-indexed
auto immediateOffset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string {
    u8 offset_8 = ((addr_mode >> 4) & 0xF0) | (addr_mode & 0xF);
    std::string sign = u ? "+" : "-";

    std::string disassembly = "[r" + std::to_string(rn);

    if(p) {
        disassembly += ", #" + sign + "0x" + common::hex(offset_8) + "]";
        disassembly += w ? "!" : "";
    } else {
        disassembly += "], #" + sign + "0x" + common::hex(offset_8);
    }

    return disassembly;
}

//LDR{<cond>}H|SH|SB <Rd>, <addressing_mode>
//STR{<cond>}H <Rd>, <addressing_mode>
auto armDisassembleHalfwordTransfer(u32 instruction, u32 address) -> std::string {
    u8 condition = instruction >> 28;
    u8 rn = (instruction >> 16) & 0xF;
    u8 rd = (instruction >> 12) & 0xF;
    bool p = (instruction >> 24) & 0x1;
    bool u = (instruction >> 23) & 0x1;
    bool i = (instruction >> 22) & 0x1;
    bool w = (instruction >> 21) & 0x1;
    bool l = (instruction >> 20) & 0x1;
    u8 sh = (instruction >> 5) & 0x3;

    assert(sh != 0);

    std::string disassembly;
    
    if(l) {
        disassembly = "ldr";
        disassembly += CONDITION_EXTENSIONS[condition];
        disassembly += sh == 1 ? "h" : sh == 2 ? "sb" : "sh";
    } else {
        //assert(sh == 1);

        disassembly = "str";
        disassembly += CONDITION_EXTENSIONS[condition];
        disassembly += "h";
    }

    disassembly += " r" + std::to_string(rd) + ", ";
    disassembly += i ? immediateOffset(rn, instruction & 0xFFF, p, u, w) : registerOffset(rn, instruction & 0xFFF, p, u, w);

    return disassembly;
}


//[<Rn>, +/-<Rm>, <shift> #<shift_imm>]
//[<Rn>, +/-<Rm>]
//[<Rn>, +/-<Rm>, <shift> #<shift_imm>]!  Pre-indexed
//[<Rn>, +/-<Rm>]!
//[<Rn>], +/-<Rm>, <shift> #<shift_imm>   Post-indexed
//[<Rn>], +/-<Rm>
auto scaledRegisterOffset(u8 rn, u16 offset, bool p, bool u, bool w) -> std::string {
    u8 rm = offset & 0xF;
    u8 shift = (offset >> 5) & 0x3;
    u8 shift_imm = (offset >> 7) & 0x1F;

    std::string disassembly;
    disassembly = "[r" + std::to_string(rn);
    disassembly += !p ? "]" : "";
    disassembly += ", ";
    disassembly += u ? '+' : '-';
    disassembly += "r" + std::to_string(rm);
    
    if(shift != 0 || shift_imm != 0) {
        disassembly += ", ";
        disassembly += shift_imm != 0 ? SHIFT_MNEMONICS[shift] : "rrx";
        if(shift_imm != 0) disassembly += " #0x" + common::hex(shift_imm);
    }

    disassembly += p ? w ? "]!" : "]" : "";

    return disassembly;
}

//[<Rn>, #+/-<offset_12>]
//[<Rn>, #+/-<offset_12>]!  Pre-indexed
//[<Rn>], #+/-<offset_12>   Post-indexed
auto immediate12Offset(u8 rn, u16 offset, bool p, bool u, bool w) -> std::string {
    std::string disassembly;
    disassembly += "[r" + std::to_string(rn);
    disassembly += !p ? "]" : "";
    disassembly += ", #";
    disassembly += u ? '+' : '-';
    disassembly += "0x" + common::hex(offset);
    disassembly += p ? w ? "]!" : "]" : "";

    return disassembly;
}

//LDR|STR{<cond>}{B}{T} <Rd>, <addressing_mode>
auto armDisassembleSingleTransfer(u32 instruction, u32 address) -> std::string {
    u8 condition = instruction >> 28;
    u8 rn = (instruction >> 16) & 0xF;
    u8 rd = (instruction >> 12) & 0xF;
    bool i = (instruction >> 25) & 0x1;
    bool p = (instruction >> 24) & 0x1;
    bool u = (instruction >> 23) & 0x1;
    bool b = (instruction >> 22) & 0x1;
    bool w = (instruction >> 21) & 0x1;
    bool l = (instruction >> 20) & 0x1;

    std::string disassembly;
    disassembly = l ? "ldr" : "str";
    disassembly += CONDITION_EXTENSIONS[condition];
    disassembly += b ? "b" : "";
    disassembly += !p && w ? "t" : "";
    disassembly += " r" + std::to_string(rd);
    disassembly += ", " + (i ? scaledRegisterOffset(rn, instruction & 0xFFF, p, u, w) : immediate12Offset(rn, instruction & 0xFFF, p, u, w));

    return disassembly;
}


auto armDisassembleUndefined(u32 instruction, u32 address) -> std::string {
    return "undefined";
}


//LDM|STM{<cond>}<addressing_mode> <Rn>{!}, <registers>{^}
//<addressing_mode> = IA/IB/DA/DB
auto armDisassembleBlockTransfer(u32 instruction, u32 address) -> std::string {
    u8 condition = instruction >> 28;
    u8 rn = (instruction >> 16) & 0xF;
    u8 pu = (instruction >> 23) & 0x3;
    bool s = (instruction >> 22) & 0x1;
    bool w = (instruction >> 21) & 0x1;
    bool l = (instruction >> 20) & 0x1;
    u16 registers = instruction & 0xFFFF;
    //assert(registers != 0); //Unpredictable

    static const char *ADDRESS_MODES[4] = {
        "da", "ia", "db", "ib"
    };

    std::string disassembly;
    disassembly = l ? "ldm" : "stm";
    disassembly += CONDITION_EXTENSIONS[condition];
    disassembly += ADDRESS_MODES[pu];
    disassembly += " r" + std::to_string(rn);
    disassembly += w ? "!" : "";
    disassembly += ", {";

    //Generate a list of the registers based on the set bits
    for(int i = 0; i < 16; i++) {
        bool set = (registers >> i) & 0x1;

        if(set) {
            //If there are 1s, or in this case registers in the list, add a comma
            disassembly += (registers & ((1 << i) - 1)) != 0 ? ", " : "";
            disassembly += "r" + std::to_string(i);
        }
    }

    disassembly += "}";
    disassembly += s ? "^" : "";

    return disassembly;
}


//B{L}{<cond>} <target_address>
auto armDisassembleBranch(u32 instruction, u32 address) -> std::string {
    u8 condition = instruction >> 28;
    bool l = (instruction >> 24) & 0x1;
    s32 immediate = instruction & 0xFFFFFF;
    immediate |= (immediate >> 23) & 0x1 ? 0xFF000000 : 0; //Sign extend 24-bit to 32-bit
    immediate <<= 2;

    std::string disassembly;
    disassembly = "b";
    disassembly += l ? "l" : "";
    disassembly += CONDITION_EXTENSIONS[condition];
    disassembly += " #0x" + common::hex(address + 8 + immediate); //This would be PC + immediate

    return disassembly;
}


//[<Rn>, #+/-<offset_8>*4]
//[<Rn>, #+/-<offset_8>*4]!
//[<Rn>], #+/-<offset_8>*4
//[<Rn>], <option>
auto addressMode5(u8 rn, u8 offset, bool p, bool u, bool w) -> std::string {
    std::string disassembly;
    disassembly = "[r" + std::to_string(rn);
    disassembly += !p ? "]" : "";
    disassembly += ", ";
    
    if(p && w) {
        //Shown as an integer in the range 0-255 surrounded by { }
        disassembly += "{" + std::to_string(offset) + "}";
    } else {
        disassembly += "#";
        disassembly += u ? '+' : '-';
        disassembly += "0x" + common::hex(offset * 4); //In assembly the offset has to be a multiple of four from 0 - 255*4
        disassembly += p ? w ? "]!" : "]" : "";
    }

    return disassembly;
}

//LDC|STC{<cond>}{L} <coproc>, <CRd>, <addressing_mode>
auto armDisassembleCoDataTransfer(u32 instruction, u32 address) -> std::string {
    u8 condition = instruction >> 28;
    u8 rn = (instruction >> 16) & 0xF;
    u8 crd = (instruction >> 12) & 0xF;
    u8 cp_num = (instruction >> 8) & 0xF;
    bool p = (instruction >> 24) & 0x1;
    bool u = (instruction >> 23) & 0x1;
    bool n = (instruction >> 22) & 0x1;
    bool w = (instruction >> 21) & 0x1;
    bool l = (instruction >> 20) & 0x1;

    std::string disassembly;
    disassembly = l ? "ldc" : "stc";
    disassembly += CONDITION_EXTENSIONS[condition];
    disassembly += n ? "l" : "";
    disassembly += " p" + std::to_string(cp_num);
    disassembly += ", c" + std::to_string(crd);
    disassembly += ", " + addressMode5(rn, instruction & 0xFF, p, u, w);

    return disassembly;
}

//CDP{<cond>} <coproc>, <opcode_1>, <CRd>, <CRn>, <CRm>, <opcode_2>
auto armDisassembleCoDataOperation(u32 instruction, u32 address) -> std::string {
    u8 condition = instruction >> 28;
    u8 opcode_1 = (instruction >> 20) & 0xF;
    u8 crn = (instruction >> 16) & 0xF;
    u8 crd = (instruction >> 12) & 0xF;
    u8 cp_num = (instruction >> 8) & 0xF;
    u8 opcode_2 = (instruction >> 5) & 0x7;
    u8 crm = instruction & 0xF;

    std::string disassembly;
    disassembly = "cdp";
    disassembly += CONDITION_EXTENSIONS[condition];
    disassembly += " p" + std::to_string(cp_num);
    disassembly += ", #" + std::to_string(opcode_1);
    disassembly += ", c" + std::to_string(crd);
    disassembly += ", c" + std::to_string(crn);
    disassembly += ", c" + std::to_string(crm);
    disassembly += ", #" + std::to_string(opcode_2);

    return disassembly;
}

//MRC|MCR{<cond>} <coproc>, <opcode_1>, <Rd>, <CRn>, <CRm>{, <opcode_2>}
auto armDisassembleCoRegisterTransfer(u32 instruction, u32 address) -> std::string {
    u8 condition = instruction >> 28;
    u8 opcode_1 = (instruction >> 21) & 0x7;
    u8 crn = (instruction >> 16) & 0xF;
    u8 rd = (instruction >> 12) & 0xF;
    u8 cp_num = (instruction >> 8) & 0xF;
    u8 opcode_2 = (instruction >> 5) & 0x7;
    u8 crm = instruction & 0xF;
    bool l = (instruction >> 20) & 0x1;

    std::string disassembly;
    disassembly = l ? "mrc" : "mcr";
    disassembly += CONDITION_EXTENSIONS[condition];
    disassembly += " p" + std::to_string(cp_num);
    disassembly += ", #" + std::to_string(opcode_1);
    disassembly += ", r" + std::to_string(rd);
    disassembly += ", c" + std::to_string(crn);
    disassembly += ", c" + std::to_string(crm);
    disassembly += opcode_2 == 0 ? "" : ", #" + std::to_string(opcode_2);

    return disassembly;
}


//SWI{<cond>} <immed_24>
auto armDisassembleSoftwareInterrupt(u32 instruction, u32 address) -> std::string {
    u8 condition = instruction >> 28;
    u32 immed_24 = instruction & 0xFFFFFF;

    std::string disassembly;
    disassembly = "swi";
    disassembly += CONDITION_EXTENSIONS[condition];
    disassembly += " #" + std::to_string(immed_24);

    return disassembly; 
}


auto armDisassembleInvalid(u32 instruction, u32 address) -> std::string {
    return "Invalid Instruction";
}


} //namespace emu