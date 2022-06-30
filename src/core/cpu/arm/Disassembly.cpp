#include "Disassembly.hpp"
#include "common/StringUtils.hpp"
#include "common/Bits.hpp"
#include <fmt/format.h>
#include <cassert>


namespace emu {

std::string (*armDisassemblyFuncs[15])(u32, u32) = {
    armDisassembleBranchExchange,
    armDisassemblePSRTransfer,
    armDisassembleDataProcessing,
    armDisassembleMultiply,
    armDisassembleMultiplyLong,
    armDisassembleSingleDataSwap,
    armDisassembleHalfwordTransfer,
    armDisassembleSingleTransfer,
    armDisassembleUndefined,
    armDisassembleBlockTransfer,
    armDisassembleBranch,
    armDisassembleCoDataTransfer,
    armDisassembleCoDataOperation,
    armDisassembleCoRegisterTransfer,
    armDisassembleSoftwareInterrupt
};

static const char *CONDITION_EXTENSIONS[16] = {
    "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
    "hi", "ls", "ge", "lt", "gt", "le", "",   "nv"
};

static const char *SHIFT_MNEMONICS[4] = {
    "lsl", "lsr", "asr", "ror"
};


//BX{<cond>} <Rn>
auto armDisassembleBranchExchange(u32 instruction, u32 address) -> std::string {
    u8 condition = bits::get<28, 4>(instruction);
    u8 rn = bits::get<0, 4>(instruction);

    return fmt::format("bx{} r{}", CONDITION_EXTENSIONS[condition], rn);
}

//MRS{<cond>} <Rd>, CPSR|SPSR
//MSR{<cond>} (CPSR|SPSR)_<fields>, #<immediate>
//MSR{<cond>} (CPSR|SPSR)_<fields>, <Rm>
auto armDisassemblePSRTransfer(u32 instruction, u32 address) -> std::string {
    u8 condition = bits::get<28, 4>(instruction);
    bool i = bits::get_bit<25>(instruction);
    bool r = bits::get_bit<22>(instruction);
    bool s = bits::get_bit<21>(instruction);
    u8 fields = bits::get<16, 4>(instruction);
    u8 rd = bits::get<12, 4>(instruction);
    std::string psr = r ? "spsr" : "cpsr";

    std::string disassembly;

    if(s) {
        u8 shift_imm = bits::get<8, 4>(instruction); //(instruction >> 8) & 0xF;
        u32 immediate = bits::ror(instruction & 0xFF, shift_imm << 1);
        std::string immed_reg = i ? fmt::format("#0x{:x}", immediate) : fmt::format("r{}", bits::get<0, 4>(instruction));
        std::string fields_affected;

        fields_affected += bits::get_bit<0>(fields) ? "c" : "";
        fields_affected += bits::get_bit<1>(fields) ? "x" : "";
        fields_affected += bits::get_bit<2>(fields) ? "s" : "";
        fields_affected += bits::get_bit<3>(fields) ? "f" : "";

        disassembly = fmt::format("msr{} {}_{}, {}", CONDITION_EXTENSIONS[condition], psr, fields_affected, immed_reg);
    } else {
        disassembly = fmt::format("mrs{} r{}, {}", CONDITION_EXTENSIONS[condition], rd, psr);
    }

    return disassembly;
}

//<Rm>, <shift> <Rs>
auto registerShift(u16 shift_operand) -> std::string {
    u8 rs = bits::get<8, 4>(shift_operand);
    u8 shift = bits::get<5, 2>(shift_operand);
    u8 rm = bits::get<0, 4>(shift_operand);

    return fmt::format("r{}, {} r{}", rm, SHIFT_MNEMONICS[shift], rs);
}

//<Rm>
//<Rm>, RRX
//<Rm>, <shift> #<shift_imm>
auto immediateShift(u16 shift_operand) -> std::string {
    u8 shift_imm = bits::get<7, 5>(shift_operand);
    u8 shift = bits::get<5, 2>(shift_operand);
    u8 rm = bits::get<0, 4>(shift_operand);

    if(shift_imm == 0) {
        if(shift == 0) {
            //Special case, just use register Rm
            return fmt::format("r{}", rm);
        } else if(shift == 3) {
            //Special case, RRX (Rotate Right with Extend)
            return fmt::format("r{}, rrx", rm);
        }
    }

    return fmt::format("r{}, {} #0x{:x}", rm, SHIFT_MNEMONICS[shift], shift_imm);
}

//#<immediate>
auto immediate(u16 shift_operand) -> std::string {
    u8 rotate_imm = bits::get<8, 4>(shift_operand) << 1; //Rotates an even number of bits
    u8 immed_8 = bits::get<0, 8>(shift_operand);
    u32 immediate = bits::ror(immed_8, rotate_imm);

    return fmt::format("#0x{:x}", immediate);
}

//AND{<cond>}{S} <Rd>, <Rn>, <shift_operand> - Math
//CMP{<cond>}{S} <Rn>, <shift_operand>       - Compare
//MOV{<cond>}{S} <Rd>, <shift_operand>       - Move
auto armDisassembleDataProcessing(u32 instruction, u32 address) -> std::string {
    static const char *OPCODE_MNEMONICS[16] = {
        "and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc",
        "tst", "teq", "cmp", "cmn", "orr", "mov", "bic", "mvn"
    };

    u8 condition = bits::get<28, 4>(instruction);
    u8 opcode = bits::get<21, 4>(instruction);
    bool s = bits::get_bit<20>(instruction);
    u8 rn = bits::get<16, 4>(instruction);
    u8 rd = bits::get<12, 4>(instruction);

    std::string disassembly = fmt::format("{}{}", OPCODE_MNEMONICS[opcode], CONDITION_EXTENSIONS[condition]);

    if(opcode < 8 || opcode == 12 || opcode == 14) {
        //<Rd>, <Rn>
        disassembly += fmt::format("{} r{}, r{}", s ? "s" : "", rd, rn);
    } else if(opcode > 7 && opcode < 12) {
        //<Rn>
        disassembly += fmt::format(" r{}", rn);
    } else if(opcode == 13 || opcode == 15) {
        //<Rd>
        disassembly += fmt::format("{} r{}", s ? "s" : "", rd);
    }

    //Check the I (immediate) bit and the 4th bit, that determines immediate shift or register shift.
    u8 addressing_mode = bits::get_bit<25>(instruction) ? 0 : bits::get_bit<4>(instruction) ? 2 : 1;

    //Shift operand is first 12 bits
    switch(addressing_mode) {
        case 0 : disassembly += ", " + immediate(instruction & 0xFFF); break;
        case 1 : disassembly += ", " + immediateShift(instruction & 0xFFF); break;
        case 2 : disassembly += ", " + registerShift(instruction & 0xFFF); break;
    }

    return disassembly;
}

//MUL{<cond>}{S} <Rd>, <Rm>, <Rs>
//MLA{<cond>}{S} <Rd>, <Rm>, <Rs>, <Rn>
auto armDisassembleMultiply(u32 instruction, u32 address) -> std::string {
    u8 condition = bits::get<28, 4>(instruction);
    bool a = bits::get_bit<21>(instruction);
    bool s = bits::get_bit<20>(instruction);
    u8 rd = bits::get<16, 4>(instruction);
    u8 rn = bits::get<12, 4>(instruction);
    u8 rs = bits::get<8, 4>(instruction);
    u8 rm = bits::get<0, 4>(instruction);

    if(a) {
        return fmt::format("mla{}{} r{}, r{}, r{}, r{}", CONDITION_EXTENSIONS[condition], s ? "s" : "", rd, rm, rs, rn);
    } else {
        return fmt::format("mul{}{} r{}, r{}, r{}", CONDITION_EXTENSIONS[condition], s ? "s" : "", rd, rm, rs);
    }
}

//UMULL{<cond>}{S} <RdLo>, <RdHi>, <Rm>, <Rs>
//UMLAL{<cond>}{S} <RdLo>, <RdHi>, <Rm>, <Rs>
//SMULL{<cond>}{S} <RdLo>, <RdHi>, <Rm>, <Rs>
//SMLAL{<cond>}{S} <RdLo>, <RdHi>, <Rm>, <Rs>
auto armDisassembleMultiplyLong(u32 instruction, u32 address) -> std::string {
    u8 condition = bits::get<28, 4>(instruction);
    bool sign = bits::get_bit<22>(instruction);
    bool a = bits::get_bit<21>(instruction);
    bool s = bits::get_bit<20>(instruction);
    u8 rd_hi = bits::get<16, 4>(instruction);
    u8 rd_lo = bits::get<12, 4>(instruction);
    u8 rs = bits::get<8, 4>(instruction);
    u8 rm = bits::get<0, 4>(instruction);

    std::string disassembly = fmt::format("{}{}", sign ? "s" : "u", a ? "mlal" : "mull");
    disassembly += fmt::format("{}{} r{}, r{}, r{}, r{}", CONDITION_EXTENSIONS[condition], s ? "s" : "", rd_lo, rd_hi, rm, rs);

    return disassembly;
}

//SWP{<cond>}{B} <Rd>, <Rm>, [Rn]
auto armDisassembleSingleDataSwap(u32 instruction, u32 address) -> std::string {
    u8 condition = bits::get<28, 4>(instruction);
    bool b = bits::get_bit<22>(instruction);
    u8 rn = bits::get<16, 4>(instruction);
    u8 rd = bits::get<12, 4>(instruction);
    u8 rm = bits::get<0, 4>(instruction);

    return fmt::format("swp{}{} r{}, r{}, [r{}]", CONDITION_EXTENSIONS[condition], b ? "b" : "", rd, rm, rn);
}

//[<Rn>, +/-<Rm>]
//[<Rn>, +/-<Rm>]!  Pre-indexed
//[<Rn>], +/-<Rm>   Post-indexed
auto registerOffset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string {
    u8 rm = bits::get<0, 4>(addr_mode);
    std::string sign = u ? "+" : "-";
    std::string disassembly = fmt::format("[r{}", rn);

    if(p) {
        disassembly += fmt::format(", {}r{}]{}", sign, rm, w ? "!" : "");
    } else {
        disassembly += fmt::format("], {}r{}", sign, rm);
    }

    return disassembly;
}

//[<Rn>, #+/-<offset_8>]
//[<Rn>, #+/-<offset_8>]!  Pre-indexed
//[<Rn>], #+/-<offset_8>   Post-indexed
auto immediateOffset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string {
    u8 offset_8 = (bits::get<8, 4>(addr_mode) << 4) | bits::get<0, 4>(addr_mode); //((addr_mode >> 4) & 0xF0) | (addr_mode & 0xF);
    std::string sign = u ? "+" : "-";
    std::string disassembly = fmt::format("[r{}", rn);

    if(p) {
        disassembly += fmt::format(", #{}0x{:x}]{}", sign, offset_8, w ? "!" : "");
    } else {
        disassembly += fmt::format("], #{}0x{:x}", sign, offset_8);
    }

    return disassembly;
}

//LDR{<cond>}H|SH|SB <Rd>, <addressing_mode>
//STR{<cond>}H <Rd>, <addressing_mode>
auto armDisassembleHalfwordTransfer(u32 instruction, u32 address) -> std::string {
    u8 condition = bits::get<28, 4>(instruction);
    bool p = bits::get_bit<24>(instruction);
    bool u = bits::get_bit<23>(instruction);
    bool i = bits::get_bit<22>(instruction);
    bool w = bits::get_bit<21>(instruction);
    bool l = bits::get_bit<20>(instruction);
    u8 rn = bits::get<16, 4>(instruction);
    u8 rd = bits::get<12, 4>(instruction);
    u8 sh = bits::get<5, 2>(instruction);
    assert(sh != 0);

    std::string disassembly;
    
    if(l) {
        disassembly = fmt::format("ldr{}{}", CONDITION_EXTENSIONS[condition], sh == 1 ? "h" : sh == 2 ? "sb" : "sh");
    } else {
        disassembly = fmt::format("str{}h", CONDITION_EXTENSIONS[condition]);
    }

    disassembly += fmt::format(" r{}, {}", rd, i ? immediateOffset(rn, instruction & 0xFFF, p, u, w) : registerOffset(rn, instruction & 0xFFF, p, u, w));

    return disassembly;
}

//[<Rn>, +/-<Rm>, <shift> #<shift_imm>]
//[<Rn>, +/-<Rm>]
//[<Rn>, +/-<Rm>, <shift> #<shift_imm>]!  Pre-indexed
//[<Rn>, +/-<Rm>]!
//[<Rn>], +/-<Rm>, <shift> #<shift_imm>   Post-indexed
//[<Rn>], +/-<Rm>
auto scaledRegisterOffset(u8 rn, u16 offset, bool p, bool u, bool w) -> std::string {
    u8 rm = bits::get<0, 4>(offset);
    u8 shift = bits::get<5, 2>(offset);
    u8 shift_imm = bits::get<7, 5>(offset);
    char sign = u ? '+' : '-';
    std::string shift_part;

    if(shift != 0 || shift_imm != 0) {
        std::string shift_mnemonic = shift_imm != 0 ? SHIFT_MNEMONICS[shift] : "rrx";
        std::string shift_amount = shift_imm != 0 ? fmt::format(" #0x{:x}", shift_imm) : "";
        shift_part = fmt::format(", {}{}", shift_mnemonic, shift_amount);
    }

    if(p) {
        return fmt::format("[r{}, {}r{}{}]{}", rn, sign, rm, shift_part, w ? "!" : "");
    } else {
        return fmt::format("[r{}], {}r{}{}", rn, sign, rm, shift_part);
    }
}

//[<Rn>, #+/-<offset_12>]
//[<Rn>, #+/-<offset_12>]!  Pre-indexed
//[<Rn>], #+/-<offset_12>   Post-indexed
auto immediate12Offset(u8 rn, u16 offset, bool p, bool u, bool w) -> std::string {
    char sign = u ? '+' : '-';

    if(p) {
        return fmt::format("[r{}, #{}0x{:x}]{}", rn, sign, offset, w ? "!" : "");
    } else {
        return fmt::format("[r{}], #{}0x{:x}", rn, sign, offset);
    }
}

//LDR|STR{<cond>}{B}{T} <Rd>, <addressing_mode>
auto armDisassembleSingleTransfer(u32 instruction, u32 address) -> std::string {
    u8 condition = bits::get<28, 4>(instruction);
    bool i = bits::get_bit<25>(instruction);
    bool p = bits::get_bit<24>(instruction);
    bool u = bits::get_bit<23>(instruction);
    bool b = bits::get_bit<22>(instruction);
    bool w = bits::get_bit<21>(instruction);
    bool l = bits::get_bit<20>(instruction);
    u8 rn = bits::get<16, 4>(instruction);
    u8 rd = bits::get<12, 4>(instruction);

    std::string disassembly;

    if(l) {
        disassembly = fmt::format("ldr{}{}{} r{}", CONDITION_EXTENSIONS[condition], b ? "b" : "", !p && w ? "t" : "", rd);
    } else {
        disassembly = fmt::format("str{}{}{} r{}", CONDITION_EXTENSIONS[condition], b ? "b" : "", !p && w ? "t" : "", rd);
    }
    disassembly += fmt::format(", {}", i ? scaledRegisterOffset(rn, instruction & 0xFFF, p, u, w) : immediate12Offset(rn, instruction & 0xFFF, p, u, w));

    return disassembly;
}

auto armDisassembleUndefined(u32 instruction, u32 address) -> std::string {
    return "undefined";
}

//LDM|STM{<cond>}<addressing_mode> <Rn>{!}, <registers>{^}
//<addressing_mode> = IA/IB/DA/DB
auto armDisassembleBlockTransfer(u32 instruction, u32 address) -> std::string {
    static const char *ADDRESS_MODES[4] = {
        "da", "ia", "db", "ib"
    };

    u8 condition = bits::get<28, 4>(instruction);
    u8 pu = bits::get<23, 2>(instruction);
    bool s = bits::get_bit<22>(instruction);
    bool w = bits::get_bit<21>(instruction);
    bool l = bits::get_bit<20>(instruction);
    u8 rn = bits::get<16, 4>(instruction);
    u16 registers = instruction & 0xFFFF;
    std::string register_list;

    std::string disassembly = fmt::format("{}{}{}", l ? "ldm" : "stm", CONDITION_EXTENSIONS[condition], ADDRESS_MODES[pu]);

    //Generate a list of the registers based on the set bits
    for(int i = 0; i < 16; i++) {
        bool set = bits::get_bit(registers, i);

        if(set) {
            //If there are 1s, or in this case registers in the list, add a comma
            register_list += bits::get(0, i, registers) != 0 ? ", " : "";
            register_list += fmt::format("r{}", i);
        }
    }
    disassembly += fmt::format(" r{}{}, {{{}}}{}", rn, w ? "!" : "", register_list, s ? "^" : "");

    return disassembly;
}

//B{L}{<cond>} <target_address>
auto armDisassembleBranch(u32 instruction, u32 address) -> std::string {
    u8 condition = bits::get<28, 4>(instruction); //instruction >> 28;
    bool l = bits::get_bit<24>(instruction); //(instruction >> 24) & 0x1;
    s32 immediate = bits::sign_extend<24, s32>(instruction & 0xFFFFFF) << 2; //Sign extend 24-bit to 32-bit

    return fmt::format("b{}{} #0x{:x}", l ? "l" : "", CONDITION_EXTENSIONS[condition], address + 8 + immediate);
}

//[<Rn>, #+/-<offset_8>*4]
//[<Rn>, #+/-<offset_8>*4]!
//[<Rn>], #+/-<offset_8>*4
//[<Rn>], <option>
auto addressMode5(u8 rn, u8 offset, bool p, bool u, bool w) -> std::string {
    char sign = u ? '+' : '-';

    if(p) {
        return fmt::format("[r{}, #{}0x{:x}]{}", rn, sign, offset * 4, w ? "!" : "");
    } else if(w) {
        return fmt::format("[r{}], #{}0x{:x}", rn, sign, offset * 4);
    } else {
        return fmt::format("[r{}], {{{}}}", rn, offset);
    }
}

//LDC|STC{<cond>}{L} <Pn>, <Cd>, <addressing_mode>
auto armDisassembleCoDataTransfer(u32 instruction, u32 address) -> std::string {
    u8 condition = bits::get<28, 4>(instruction);
    bool p = bits::get_bit<24>(instruction);
    bool u = bits::get_bit<23>(instruction);
    bool n = bits::get_bit<22>(instruction);
    bool w = bits::get_bit<21>(instruction);
    bool l = bits::get_bit<20>(instruction);
    u8 rn = bits::get<16, 4>(instruction);
    u8 cd = bits::get<12, 4>(instruction);
    u8 pn = bits::get<8, 4>(instruction);
    std::string address_mode = addressMode5(rn, instruction & 0xFF, p, u, w);

    return fmt::format("{}{}{} p{}, c{}, {}", l ? "ldc" : "stc", CONDITION_EXTENSIONS[condition], n ? "l" : "", pn, cd, address_mode);
}

//CDP{<cond>} <Pn>, <opcode>, <Cd>, <Cn>, <Cm>{, <cp>}
auto armDisassembleCoDataOperation(u32 instruction, u32 address) -> std::string {
    u8 condition = bits::get<28, 4>(instruction);
    u8 opcode = bits::get<20, 4>(instruction);
    u8 cn = bits::get<16, 4>(instruction);
    u8 cd = bits::get<12, 4>(instruction);
    u8 pn = bits::get<8, 4>(instruction);
    u8 cp = bits::get<5, 3>(instruction);
    u8 cm = bits::get<0, 4>(instruction);
    std::string info = cp == 0 ? fmt::format(", #{}", cp) : "";

    return fmt::format("cdp{} p{}, #{}, c{}, c{}, c{}{}", CONDITION_EXTENSIONS[condition], pn, opcode, cd, cn, cm, info);
}

//MRC|MCR{<cond>} <Pn>, <opcode>, <Rd>, <Cn>, <Cm>{, <cp>}
auto armDisassembleCoRegisterTransfer(u32 instruction, u32 address) -> std::string {
    u8 condition = bits::get<28, 4>(instruction);
    u8 opcode = bits::get<21, 3>(instruction);
    bool l = bits::get_bit<20>(instruction);
    u8 cn = bits::get<16, 4>(instruction);
    u8 rd = bits::get<12, 4>(instruction);
    u8 pn = bits::get<8, 4>(instruction);
    u8 cp = bits::get<5, 3>(instruction);
    u8 cm = bits::get<0, 4>(instruction);
    std::string info = cp == 0 ? fmt::format(", #{}", cp) : "";

    return fmt::format("{}{} p{}, #{}, r{}, c{}, c{}{}", l ? "mrc" : "mcr", CONDITION_EXTENSIONS[condition], pn, opcode, rd, cn, cm, info);
}

//SWI{<cond>} <immed_24>
auto armDisassembleSoftwareInterrupt(u32 instruction, u32 address) -> std::string {
    u8 condition = bits::get<28, 4>(instruction);
    u32 immed_24 = instruction & 0xFFFFFF;

    return fmt::format("swi{} #{}", CONDITION_EXTENSIONS[condition], immed_24);
}

} //namespace emu