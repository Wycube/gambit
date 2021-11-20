#include "Dissasembly.hpp"
#include "common/Utilities.hpp"

#include <cassert>


//TODO: 
// - Some instructions have PC relative addressing, they will need the PC to be passed in.
// - Use a formatting library like {fmt}
// - Add some extra syntax like {r0-r3} for ldm/stm, or ldmia as ldm

namespace emu {

//0b1111 is obselete on armv4 and is unpredictable
//but I'll still use NV mnemonic extension to identify them.
static const char *CONDITION_EXTENSIONS[16] = {
    "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
    "hi", "ls", "ge", "lt", "gt", "le", "",   "nv"
};

static const char *SHIFT_MNEMONICS[4] = {
    "lsl", "lsr", "asr", "ror"
};


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
auto dissasembleDataProcessing(u32 instruction) -> std::string {
    static const char *OPCODE_MNEMONICS[16] = {
        "and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc",
        "tst", "teq", "cmp", "cmn", "orr", "mov", "bic", "mvn"
    };

    u8 condition = instruction >> 28;
    u8 opcode = (instruction >> 21) & 0xF;
    u8 rn = (instruction >> 16) & 0xF;
    u8 rd = (instruction >> 12) & 0xF;

    std::string diss;
    diss += OPCODE_MNEMONICS[opcode]; 
    diss += CONDITION_EXTENSIONS[condition];
    diss += (instruction >> 20) & 0x1 ? "s" : ""; //Save to CPSR bit

    if(opcode < 8 || opcode == 12 || opcode == 14) {
        //<Rd>, <Rn>
        diss += " r" + std::to_string(rd);
        diss += ", r" + std::to_string(rn);
    } else if(opcode > 7 && opcode < 12) {
        //<Rn>
        diss += " r" + std::to_string(rn);
    } else if(opcode == 13 || opcode == 15) {
        //<Rd>
        diss += " r" + std::to_string(rd);
    }

    //Check the I (immediate) bit and the 4th bit, that determines immediate shift or register shift.
    u8 addressing_mode = (instruction >> 25) & 0x1 ? 0 : (instruction >> 4) & 0x1 ? 2 : 1;

    //Shift operand is first 12 bits
    switch(addressing_mode) {
        case 0 : diss += ", " + immediate(instruction & 0xFFF);
        break;
        case 1 : diss += ", " + immediateShift(instruction & 0xFFF);
        break;
        case 2 : diss += ", " + registerShift(instruction & 0xFFF);
        break;
    }

    return diss;
}


//MUL{<cond>}{S} <Rd>, <Rm>, <Rs>
//MLA{<cond>}{S} <Rd>, <Rm>, <Rs>
auto dissasembleMultiply(u32 instruction) -> std::string {
    u8 condition = instruction >> 28;
    u8 rd = (instruction >> 16) & 0xF;
    u8 rn = (instruction >> 12) & 0xF;
    u8 rs = (instruction >> 8) & 0xF;
    u8 rm = instruction & 0xF;
    bool accumulate = (instruction >> 21) & 0x1;

    std::string diss;
    diss += accumulate ? "mla" : "mul"; //Accumulate bit
    diss += CONDITION_EXTENSIONS[condition];
    diss += (instruction >> 20) & 0x1 ? "s" : ""; //Save to CPSR bit
    diss += " r" + std::to_string(rd);
    diss += ", r" + std::to_string(rm);
    diss += ", r" + std::to_string(rs);
    diss += accumulate ? ", r" + std::to_string(rn) : "";

    return diss;
}

//UMULL{<cond>}{S} <RdLo>, <RdHi>, <Rm>, <Rs>
//UMLAL{<cond>}{S} <RdLo>, <RdHi>, <Rm>, <Rs>
//SMULL{<cond>}{S} <RdLo>, <RdHi>, <Rm>, <Rs>
//SMLAL{<cond>}{S} <RdLo>, <RdHi>, <Rm>, <Rs>
auto dissasembleMultiplyLong(u32 instruction) -> std::string {
    u8 condition = instruction >> 28;
    u8 rd_hi = (instruction >> 16) & 0xF;
    u8 rd_lo = (instruction >> 12) & 0xF;
    u8 rs = (instruction >> 8) & 0xF;
    u8 rm = instruction & 0xF;

    std::string diss;
    diss += (instruction >> 22) & 0x1 ? "s" : "u"; //Unsigned bit
    diss += (instruction >> 21) & 0x1 ? "mlal" : "mull"; //Accumulate bit
    diss += CONDITION_EXTENSIONS[condition];
    diss += (instruction >> 20) & 0x1 ? "s" : ""; //Save to CPSR bit
    diss += " r" + std::to_string(rd_lo);
    diss += ", r" + std::to_string(rd_hi);
    diss += ", r" + std::to_string(rm);
    diss += ", r" + std::to_string(rs);

    return diss;
}


//SWP{<cond>}{B} <Rd>, <Rm>, [Rn]
auto dissasembleDataSwap(u32 instruction) -> std::string {
    u8 condition = instruction >> 28;
    u8 rn = (instruction >> 16) & 0xF;
    u8 rd = (instruction >> 12) & 0xF;
    u8 rm = instruction & 0xF;

    std::string diss = "swp";
    diss += CONDITION_EXTENSIONS[condition];
    diss += (instruction >> 22) & 0x1 ? "b" : ""; //Byte bit
    diss += " r" + std::to_string(rd);
    diss += ", r" + std::to_string(rm);
    diss += ", [r" + std::to_string(rn) + "]";

    return diss;
}


//BX{<cond>} <Rm>
auto dissasembleBranchExchange(u32 instruction) -> std::string {
    u8 condition = instruction >> 28;
    u8 rm = instruction & 0xF;

    std::string diss = "bx";
    diss += CONDITION_EXTENSIONS[condition];
    diss += " r" + std::to_string(rm);

    return diss;
}

//[<Rn>, +/-<Rm>]
//[<Rn>, +/-<Rm>]!  Pre-indexed
//[<Rn>], +/-<Rm>   Post-indexed
auto registerOffset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string {
    u8 rm = addr_mode & 0xF;
    std::string sign = u ? "+" : "-";

    std::string diss = "[r" + std::to_string(rn);

    if(p) {
        diss += ", " + sign + "r" + std::to_string(rm) + "]";
        diss += w ? "!" : "";
    } else {
        diss += "], " + sign + "r" + std::to_string(rm);
    }

    return diss;
}

//[<Rn>, #+/-<offset_8>]
//[<Rn>, #+/-<offset_8>]!  Pre-indexed
//[<Rn>], #+/-<offset_8>   Post-indexed
auto immediateOffset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string {
    u8 offset_8 = ((addr_mode >> 4) & 0xF0) | (addr_mode & 0xF);
    std::string sign = u ? "+" : "-";

    std::string diss = "[r" + std::to_string(rn);

    if(p) {
        diss += ", " + sign + "#0x" + common::hex(offset_8) + "]";
        diss += w ? "!" : "";
    } else {
        diss += "], " + sign + "#0x" + common::hex(offset_8);
    }

    return diss;
}

//LDR{<cond>}H|SH|SB <Rd>, <addressing_mode>
//STR{<cond>}H <Rd>, <addressing_mode>
auto dissasembleHalfwordTransfer(u32 instruction) -> std::string {
    u8 condition = instruction >> 28;
    u8 rn = (instruction >> 16) & 0xF;
    u8 rd = (instruction >> 12) & 0xF;
    bool p = (instruction >> 24) & 0x1;
    bool u = (instruction >> 23) & 0x1;
    bool w = (instruction >> 21) & 0x1;
    bool i = ((instruction >> 21) & 0x2) | (instruction >> 22) & 0x1;
    bool l = (instruction >> 20) & 0x1;
    u8 sh = (instruction >> 5) & 0x3;

    assert(sh != 0);

    std::string diss;
    
    if(l) {
        diss = "ldr";
        diss += CONDITION_EXTENSIONS[condition];
        diss += sh == 1 ? "h" : sh == 2 ? "sb" : "sh";
    } else {
        assert(sh == 1);

        diss = "str";
        diss += CONDITION_EXTENSIONS[condition];
        diss += "h";
    }

    diss += " r" + std::to_string(rd) + ", ";
    diss += i ? immediateOffset(rn, instruction & 0xFFF, p, u, w) : registerOffset(rn, instruction & 0xFFF, p, u, w);

    return diss;
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

    std::string diss;
    diss = "[r" + std::to_string(rn);
    diss += !p ? "]" : "";
    diss += ", ";
    diss += u ? '+' : '-';
    diss += "r" + std::to_string(rm);
    
    if(shift != 0 || shift_imm != 0) {
        diss += ", ";
        diss += shift_imm != 0 ? SHIFT_MNEMONICS[shift] : "rrx";
        diss += " #0x" + common::hex(shift_imm);
    }

    diss += p ? w ? "]!" : "]" : "";

    return diss;
}

//[<Rn>, #+/-<offset_12>]
//[<Rn>, #+/-<offset_12>]!  Pre-indexed
//[<Rn>], #+/-<offset_12>   Post-indexed
auto immediate12Offset(u8 rn, u16 offset, bool p, bool u, bool w) -> std::string {
    std::string diss;
    diss += "[r" + std::to_string(rn);
    diss += !p ? "]" : "";
    diss += ", #";
    diss += u ? '+' : '-';
    diss += "0x" + common::hex(offset);
    diss += p ? w ? "]!" : "]" : "";

    return diss;
}

//LDR|STR{<cond>}{B}{T} <Rd>, <addressing_mode>
auto dissasembleSingleTransfer(u32 instruction) -> std::string {
    u8 condition = instruction >> 28;
    u8 rn = (instruction >> 16) & 0xF;
    u8 rd = (instruction >> 12) & 0xF;
    bool i = (instruction >> 25) & 0xF;
    bool p = (instruction >> 24) & 0x1;
    bool u = (instruction >> 23) & 0x1;
    bool b = (instruction >> 22) & 0x1;
    bool w = (instruction >> 21) & 0x1;
    bool l = (instruction >> 20) & 0x1;

    std::string diss;
    diss = l ? "ldr" : "str";
    diss += CONDITION_EXTENSIONS[condition];
    diss += b ? "b" : "";
    diss += !p && w ? "t" : "";
    diss += " r" + std::to_string(rd);
    diss += i ? scaledRegisterOffset(rn, instruction & 0xFFF, p, u, w) : immediate12Offset(rn, instruction & 0xFFF, p, u, w);

    return diss;
}


auto dissasembleUndefined(u32 instruction) -> std::string {
    return "undefined";
}


//LDM|STM{<cond>}<addressing_mode> <Rn>{!}, <registers>{^}
//<addressing_mode> = IA/IB/DA/DB
auto dissasembleBlockTransfer(u32 instruction) -> std::string {
    u8 condition = instruction >> 28;
    u8 rn = (instruction >> 16) & 0xF;
    u8 pu = (instruction >> 23) & 0x3;
    bool s = (instruction >> 22) & 0x1;
    bool w = (instruction >> 21) & 0x1;
    bool l = (instruction >> 20) & 0x1;
    u16 registers = instruction & 0xFFFF;
    assert(registers != 0); //Unpredictable

    static const char *ADDRESS_MODES[4] = {
        "da", "ia", "db", "da"
    };

    std::string diss;
    diss = l ? "ldm" : "stm";
    diss += CONDITION_EXTENSIONS[condition];
    diss += ADDRESS_MODES[pu];
    diss += " r" + std::to_string(rn);
    diss += w ? "!" : "";
    diss += ", {";

    //Generate a list of the registers based on the set bits
    for(int i = 0; i < 16; i++) {
        bool set = (registers >> i) & 0x1;

        if(set) {
            //If there are 1s, or in this case registers in the list, add a comma
            diss += (registers & ((1 << i) - 1)) != 0 ? ", " : "";
            diss += "r" + std::to_string(i);
        }
    }

    diss += "}";
    diss += s ? "^" : "";

    return diss;
}


//B{L}{<cond>} <target_address>
auto dissasembleBranch(u32 instruction) -> std::string {
    u8 condition = instruction >> 28;
    bool l = (instruction >> 24) & 0x1;
    s32 immediate = instruction & 0xFFFFFF;
    immediate |= (immediate >> 23) & 0x1 ? 0xFF000000 : 0; //Sign extend 24-bit to 32-bit
    immediate <<= 2;

    std::string diss;
    diss = "b";
    diss += l ? "l" : "";
    diss += CONDITION_EXTENSIONS[condition];
    diss += " #0x" + common::hex(8 + immediate); //This would be PC + immediate or instruction_address + 8 + immediate

    return diss;
}


//[<Rn>, #+/-<offset_8>*4]
//[<Rn>, #+/-<offset_8>*4]!
//[<Rn>], #+/-<offset_8>*4
//[<Rn>], <option>
auto addressMode5(u8 rn, u8 offset, bool p, bool u, bool w) -> std::string {
    u8 pw = (p << 1) | w;

    std::string diff;
    diff = "[r" + std::to_string(rn);
    diff += !p ? "]" : "";
    diff += ", ";
    
    if(p && w) {
        //Shown as an integer in the range 0-255 surrounded by { }
        diff += "{" + std::to_string(offset) + "}";
    } else {
        diff += "#";
        diff += u ? '+' : '-';
        diff += "0x" + common::hex(offset * 4); //In assembly the offset has to be a multiple of four from 0 - 255*4
        diff += p ? w ? "]!" : "]" : "";
    }

    return diff;
}

//LDC|STC{<cond>}{L} <coproc>, <CRd>, <addressing_mode>
auto dissasembleCoDataTransfer(u32 instruction) -> std::string {
    u8 condition = instruction >> 28;
    u8 rn = (instruction >> 16) & 0xF;
    u8 crd = (instruction >> 12) & 0xF;
    u8 cp_num = (instruction >> 8) & 0xF;
    bool p = (instruction >> 24) & 0x1;
    bool u = (instruction >> 23) & 0x1;
    bool n = (instruction >> 22) & 0x1;
    bool w = (instruction >> 21) & 0x1;
    bool l = (instruction >> 20) & 0x1;

    std::string diss;
    diss = l ? "ldc" : "stc";
    diss += CONDITION_EXTENSIONS[condition];
    diss += n ? "l" : "";
    diss += " p" + std::to_string(cp_num);
    diss += ", c" + std::to_string(crd);
    diss += ", " + addressMode5(rn, instruction & 0xFF, p, u, w);

    return diss;
}

//CDP{<cond>} <coproc>, <opcode_1>, <CRd>, <CRn>, <CRm>, <opcode_2>
auto dissasembleCoDataOperation(u32 instruction) -> std::string {
    u8 condition = instruction >> 28;
    u8 opcode_1 = (instruction >> 20) & 0xF;
    u8 crn = (instruction >> 16) & 0xF;
    u8 crd = (instruction >> 12) & 0xF;
    u8 cp_num = (instruction >> 8) & 0xF;
    u8 opcode_2 = (instruction >> 5) & 0x7;
    u8 crm = instruction & 0xF;

    std::string diss;
    diss = "cdp";
    diss += CONDITION_EXTENSIONS[condition];
    diss += " p" + std::to_string(cp_num);
    diss += ", #" + std::to_string(opcode_1);
    diss += ", c" + std::to_string(crd);
    diss += ", c" + std::to_string(crn);
    diss += ", c" + std::to_string(crm);
    diss += ", #" + std::to_string(opcode_2);

    return diss;
}

//MRC|MCR{<cond>} <coproc>, <opcode_1>, <Rd>, <CRn>, <CRm>{, <opcode_2>}
auto dissasembleCoRegisterTransfer(u32 instruction) -> std::string {
    u8 condition = instruction >> 28;
    u8 opcode_1 = (instruction >> 21) & 0x7;
    u8 crn = (instruction >> 16) & 0xF;
    u8 rd = (instruction >> 12) & 0xF;
    u8 cp_num = (instruction >> 8) & 0xF;
    u8 opcode_2 = (instruction >> 5) & 0x7;
    u8 crm = instruction & 0xF;
    bool l = (instruction >> 20) & 0x1;

    std::string diss;
    diss = l ? "mrc" : "mcr";
    diss += CONDITION_EXTENSIONS[condition];
    diss += " p" + std::to_string(cp_num);
    diss += ", #" + std::to_string(opcode_1);
    diss += ", r" + std::to_string(rd);
    diss += ", c" + std::to_string(crn);
    diss += ", c" + std::to_string(crm);
    diss += opcode_2 == 0 ? "" : ", #" + std::to_string(opcode_2);

    return diss;
}


//SWI{<cond>} <immed_24>
auto dissasembleSoftwareInterrupt(u32 instruction) -> std::string {
    u8 condition = instruction >> 28;
    u32 immed_24 = instruction & 0xFFFFFF;

    std::string diss;
    diss = "swi";
    diss += CONDITION_EXTENSIONS[condition];
    diss += " #0x" + common::hex(immed_24);

    return diss; 
}

} //namespace emu