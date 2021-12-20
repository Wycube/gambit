#include "Disassembly.hpp"
#include "common/StringUtils.hpp"

#include <cassert>


namespace emu {

//LSL|LSR|ASR <Rd>, <Rm>, #<immed_5>
auto disassembleMoveShifted(u16 instruction) -> std::string {
    static const char *OPCODE_MNEMONICS[] = {
        "lsl", "lsr", "asr"
    };

    u8 opcode = (instruction >> 11) & 0x3;
    u8 immed_5 = (instruction >> 6) & 0x1F;
    u8 rm = (instruction >> 3) & 0x7;
    u8 rd = instruction & 0x7;

    std::string disassembly;

    disassembly = OPCODE_MNEMONICS[opcode];
    disassembly += " r" + std::to_string(rd);
    disassembly += ", r" + std::to_string(rm);
    disassembly += ", #" + std::to_string(immed_5 == 0 ? 32 : immed_5);

    return disassembly;
}


//ADD|SUB <Rd>, <Rn>, #<immed_3>
//ADD|SUB <Rd>, <Rn>, <Rm>
auto disassembleAddSubtract(u16 instruction) -> std::string {
    bool i = (instruction >> 10) & 0x1;
    bool s = (instruction >> 9) & 0x1;
    u8 rm_immed = (instruction >> 6) & 0x7;
    u8 rn = (instruction >> 3) & 0x7;
    u8 rd = instruction & 0x7;

    std::string disassembly;
    disassembly = s ? "sub" : "add";
    disassembly += " r" + std::to_string(rd);
    disassembly += ", r" + std::to_string(rn);
    disassembly += i ? ", #" + std::to_string(rm_immed) : ", r" + std::to_string(rm_immed);

    return disassembly;
}

//MOV|CMP|ADD|SUB <Rd/Rn>, #<immed_8>
auto disassembleProcessImmediate(u16 instruction) -> std::string {
    static const char *OPCODE_MNEMONICS[] = {
        "mov", "cmp", "add", "sub"
    };

    u8 opcode = (instruction >> 11) & 0x3;
    u8 rd_rn = (instruction >> 8) & 0x7;
    u8 immed_8 = instruction & 0xFF;

    std::string disassembly;
    disassembly = OPCODE_MNEMONICS[opcode];
    disassembly += " r" + std::to_string(rd_rn);
    disassembly += ", #0x" + common::hex(immed_8);

    return disassembly;
}


//<opcode> <Rd/Rn>, <Rm>
auto disassembleALUOperation(u16 instruction) -> std::string {
    static const char *OPCODE_MNEMONICS[] = {
        "and", "eor", "lsl", "lsr", "asr", "adc", "sbc", "ror",
        "tst", "neg", "cmp", "cmn", "orr", "mul", "bic", "mvn"
    };

    u8 opcode = (instruction >> 6) & 0xF;
    u8 rm = (instruction >> 3) & 0x7;
    u8 rd_rn = instruction & 0x7;

    std::string disassembly;
    disassembly = OPCODE_MNEMONICS[opcode];
    disassembly += " r" + std::to_string(rd_rn);
    disassembly += ", r" + std::to_string(rm);

    return disassembly;
}


//ADD|CMP|MOV <Rd/Rn>, <Rm>
auto disassembleHiRegOperation(u16 instruction) -> std::string {
    static const char *OPCODE_MNEMONICS[] = {
        "add", "cmp", "mov"
    };

    u8 opcode = (instruction >> 8) & 0x3;
    u8 rm = (instruction >> 3) & 0xF;
    u8 rd_rn = ((instruction >> 4) & 0x8) | (instruction & 0x7);

    assert(opcode != 0x3);

    std::string disassembly;
    disassembly = OPCODE_MNEMONICS[opcode];
    disassembly += " r" + std::to_string(rd_rn);
    disassembly += ", r" + std::to_string(rm);

    return disassembly;
}


//BLX <Rm>
auto disassembleBranchExchange(u16 instruction) -> std::string {
    u8 rm = (instruction >> 3) & 0xF;

    std::string disassembly;
    disassembly = "blx";
    disassembly += " r" + std::to_string(rm);

    return disassembly;
}


//LDR <Rd>, [PC, #<immed_8>*4]
auto disassemblePCRelativeLoad(u16 instruction) -> std::string {
    u8 rd = (instruction >> 8) & 0x7;
    u8 immed_8 = instruction & 0xFF;

    std::string disassembly;
    disassembly = "ldr";
    disassembly += " r" + std::to_string(rd);
    disassembly += ", [pc, #0x" + common::hex(immed_8 << 2) + "]";

    return disassembly;
}


//LDR|STR{B} <Rd>, [<Rn>, <Rm>]
auto disassembleLoadStoreRegister(u16 instruction) -> std::string {
    bool l = (instruction >> 11) & 0x1;
    bool b = (instruction >> 10) & 0x1;
    u8 rm = (instruction >> 6) & 0x7;
    u8 rn = (instruction >> 3) & 0x7;
    u8 rd = instruction & 0x7;

    std::string disassembly;
    disassembly = l ? "ldr" : "str";
    disassembly += b ? "b" : "";
    disassembly += " r" + std::to_string(rd);
    disassembly += ", [r" + std::to_string(rn);
    disassembly += ", r" + std::to_string(rm) + "]";

    return disassembly;
}


//STRH|LDRH|LDSB|LDSH <Rd>, [<Rn>, <Rm>]
auto disassembleLoadStoreSigned(u16 instruction) -> std::string {
    static const char *MNEMONICS[] = {
        "strh", "ldsb", "ldrh", "ldsh"
    };

    u8 hs = (instruction >> 10) & 0x3;
    u8 rm = (instruction >> 6) & 0x7;
    u8 rn = (instruction >> 3) & 0x7;
    u8 rd = instruction & 0x7;

    std::string disassembly;
    disassembly = MNEMONICS[hs];
    disassembly += " r" + std::to_string(rd);
    disassembly += ", [r" + std::to_string(rn);
    disassembly += ", r" + std::to_string(rm) + "]";

    return disassembly;
}


//STR|LDR{B} <Rd>, [<Rn>, #<immed_5>]
auto disassembleLoadStoreImmediate(u16 instruction) -> std::string {
    bool b = (instruction >> 12) & 0x1;
    bool l = (instruction >> 11) & 0x1;
    u8 immed_5 = (instruction >> 6) & 0x1F;
    u8 rn = (instruction >> 3) & 0x7;
    u8 rd = instruction & 0x7;

    std::string disassembly;
    disassembly = l ? "ldr" : "str";
    disassembly += b ? "b" : "";
    disassembly += " r" + std::to_string(rd);
    disassembly += ", [r" + std::to_string(rn);
    disassembly += ", #" + std::to_string(immed_5) + "]";

    return disassembly;
}


//STRH|LDRH <Rd>, [<Rn>, #<immed_5>*2]
auto disassembleLoadStoreHalfword(u16 instruction) -> std::string {
    bool l = (instruction >> 11) & 0x1;
    u8 immed_5 = (instruction >> 6) & 0x1F;
    u8 rn = (instruction >> 3) & 0x7;
    u8 rd = instruction & 0x7;

    std::string disassembly;
    disassembly = l ? "ldrh" : "strh";
    disassembly += " r" + std::to_string(rd);
    disassembly += ", [r" + std::to_string(rn);
    disassembly += ", #" + std::to_string(immed_5 << 1) + "]";

    return disassembly;
}


//STR|LDR <Rd>, [SP, #<immed_8>*4]
auto disassembleSPRelativeLoadStore(u16 instruction) -> std::string {
    bool l = (instruction >> 11) & 0x1;
    u8 rd = (instruction >> 8) & 0x7;
    u8 immed_8 = instruction & 0xFF;

    std::string disassembly;
    disassembly = l ? "ldr" : "str";
    disassembly += " r" + std::to_string(rd);
    disassembly += ", [sp, #0x" + common::hex(immed_8 << 2) + "]";

    return disassembly;
}


//ADD <Rd>, PC|SP, #<immed_8>*4
auto disassembleLoadAddress(u16 instruction) -> std::string {
    bool sp = (instruction >> 11) & 0x1;
    u8 rd = (instruction >> 8) & 0x7;
    u8 immed_8 = instruction & 0xFF;

    std::string disassembly;
    disassembly = "add";
    disassembly += " r" + std::to_string(rd);
    disassembly += ", " + sp ? "sp" : "pc";
    disassembly += ", #0x" + common::hex(immed_8 << 2);

    return disassembly;
}


//ADD SP, #{-}<immed_7>*4
auto disassembleAdjustSP(u16 instruction) -> std::string {
    bool s = (instruction >> 7) & 0x1;
    u8 immed_7 = instruction & 0x7F;

    std::string disassembly;
    disassembly = "add sp, ";
    disassembly += "#" + s ? "-" : "";
    disassembly += std::to_string(immed_7 << 2);

    return disassembly;
}


//PUSH <registers> - If the R bit is set then LR is in the register list
//POP <registers>  - If the R bit is set then PC is in the register list
auto disassemblePushPopRegisters(u16 instruction) -> std::string {
    bool l = (instruction >> 11) & 0x1;
    bool r = (instruction >> 8) & 0x1;
    u8 registers = instruction & 0xFF;

    std::string disassembly;
    disassembly = l ? "pop" : "push";
    disassembly += " {";
    
    //Generate a list of the registers based on the set bits
    for(int i = 0; i < 8; i++) {
        bool set = (registers >> i) & 0x1;

        if(set) {
            //If there are 1s, or in this case registers in the list, add a comma
            disassembly += (registers & ((1 << i) - 1)) != 0 ? ", " : "";
            disassembly += "r" + std::to_string(i);
        }
    }

    disassembly += r ? l ? ", pc}" : ", lr}" : "";

    return disassembly;
}


//STMIA|LDMIA <Rn>!, <registers>
auto disassembleLoadStoreMultiple(u16 instruction) -> std::string {
    bool l = (instruction >> 11) & 0x1;
    u8 rn = (instruction >> 8) & 0x7;
    u8 registers = instruction & 0xFF;

    std::string disassembly;
    disassembly = l ? "stmia" : "ldmia";
    disassembly += ", r" + std::to_string(rn) + "!";
    disassembly += " {";

    //Generate a list of the registers based on the set bits
    for(int i = 0; i < 8; i++) {
        bool set = (registers >> i) & 0x1;

        if(set) {
            //If there are 1s, or in this case registers in the list, add a comma
            disassembly += (registers & ((1 << i) - 1)) != 0 ? ", " : "";
            disassembly += "r" + std::to_string(i);
        }
    }

    disassembly += "}";

    return disassembly;
}


//B<cond> <target_address>
auto disassembleUnconditionalBranch(u16 instruction) -> std::string {
    static const char *CONDITION_CODES[] = {
        "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
        "hi", "ls", "ge", "lt", "gt", "le", "",   "nv"
    };

    u8 condition = (instruction >> 8) & 0xF;
    s32 immediate = instruction & 0xFF;
    immediate <<= 1;
    immediate |= (immediate >> 8) & 0x1 ? 0xFFFFFF00 : 0; //Sign extend 8-bit to 32-bit

    std::string disassembly;
    disassembly = "b";
    disassembly += CONDITION_CODES[condition];
    disassembly += ", " + std::to_string(4 + immediate);

    return disassembly;
}


//SWI <immed_8>
auto disassembleSoftwareInterrupt(u16 instruction) -> std::string {
    u8 immed_8 = instruction & 0xFF;

    std::string disassembly;
    disassembly = "swi";
    disassembly += " " + std::to_string(immed_8);

    return disassembly;
}


//B <target_address>
auto disassembleUnconditionalBranch(u16 instruction) -> std::string {
    s32 immediate = instruction & 0xFFF;
    immediate <<= 1;
    immediate |= (immediate << 11) & 0x1 ? 0xFFFFF000 : 0; //Sign extend 11-bit to 32-bit

    std::string disassembly;
    disassembly = "b";
    disassembly += " " + std::to_string(4 + immediate);

    return disassembly;
}


//BL <target_address>
auto disassembleLongBranch(u16 instruction) -> std::string {
    //I don't really know what to do for this one yet
}


auto disassembleUndefined(u16 instruction) -> std::string {
    return "undefined";
}

} //namespace emu