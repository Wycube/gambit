#include "Disassembly.hpp"
#include "common/StringUtils.hpp"
#include "common/Bits.hpp"
#include "common/Log.hpp"
#include <cassert>


namespace emu {

std::string (*thumbDisassemblyFuncs[21])(u16, u32, u16) = {
    thumbDisassembleMoveShifted,
    thumbDisassembleAddSubtract,
    thumbDisassembleProcessImmediate,
    thumbDisassembleALUOperation,
    thumbDisassembleHiRegisterOp,
    thumbDisassembleBranchExchange,
    thumbDisassemblePCRelativeLoad,
    thumbDisassembleLoadStoreRegister,
    thumbDisassembleLoadStoreSigned,
    thumbDisassembleLoadStoreImmediate,
    thumbDisassembleLoadStoreHalfword,
    thumbDisassembleSPRelativeLoadStore,
    thumbDisassembleLoadAddress,
    thumbDisassembleAdjustSP,
    thumbDisassemblePushPopRegisters,
    thumbDisassembleLoadStoreMultiple,
    thumbDisassembleConditionalBranch,
    thumbDisassembleSoftwareInterrupt,
    thumbDisassembleUnconditionalBranch,
    thumbDisassembleLongBranch,
    thumbDisassembleInvalid
};


//LSL|LSR|ASR <Rd>, <Rm>, #<immed_5>
auto thumbDisassembleMoveShifted(u16 instruction, u32 address, u16 prev) -> std::string {
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
    disassembly += ", #0x" + common::hex(immed_5 == 0 ? 32 : immed_5);

    return disassembly;
}

//ADD|SUB <Rd>, <Rn>, #<immed_3>
//ADD|SUB <Rd>, <Rn>, <Rm>
auto thumbDisassembleAddSubtract(u16 instruction, u32 address, u16 prev) -> std::string {
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
auto thumbDisassembleProcessImmediate(u16 instruction, u32 address, u16 prev) -> std::string {
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
auto thumbDisassembleALUOperation(u16 instruction, u32 address, u16 prev) -> std::string {
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
auto thumbDisassembleHiRegisterOp(u16 instruction, u32 address, u16 prev) -> std::string {
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

//BX|BLX <Rm>
auto thumbDisassembleBranchExchange(u16 instruction, u32 address, u16 prev) -> std::string {
    bool link = (instruction >> 7) & 0x1;
    u8 rm = (instruction >> 3) & 0xF;

    std::string disassembly;
    disassembly = link ? "blx" : "bx";
    disassembly += " r" + std::to_string(rm);

    return disassembly;
}

//LDR <Rd>, [PC, #<immed_8>*4]
auto thumbDisassemblePCRelativeLoad(u16 instruction, u32 address, u16 prev) -> std::string {
    u8 rd = (instruction >> 8) & 0x7;
    u8 immed_8 = instruction & 0xFF;

    std::string disassembly;
    disassembly = "ldr";
    disassembly += " r" + std::to_string(rd);
    disassembly += ", [pc, #0x" + common::hex(immed_8 * 4) + "]";

    return disassembly;
}

//STR|LDR{B} <Rd>, [<Rn>, <Rm>]
auto thumbDisassembleLoadStoreRegister(u16 instruction, u32 address, u16 prev) -> std::string {
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

//STRH|LDRSB|LDRH|LDRSH <Rd>, [<Rn>, <Rm>]
auto thumbDisassembleLoadStoreSigned(u16 instruction, u32 address, u16 prev) -> std::string {
    static const char *MNEMONICS[] = {
        "strh", "ldrsb", "ldrh", "ldrsh"
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

//STR|LDR <Rd>, [<Rn>, #<immed_5>*4]
//STRB|LDRB <Rd>, [<Rn>, #<immed_5>]
auto thumbDisassembleLoadStoreImmediate(u16 instruction, u32 address, u16 prev) -> std::string {
    bool b = (instruction >> 12) & 0x1;
    bool l = (instruction >> 11) & 0x1;
    u8 immed_5 = (instruction >> 6) & 0x1F;
    immed_5 <<= b ? 0 : 2;
    u8 rn = (instruction >> 3) & 0x7;
    u8 rd = instruction & 0x7;

    std::string disassembly;
    disassembly = l ? "ldr" : "str";
    disassembly += b ? "b" : "";
    disassembly += " r" + std::to_string(rd);
    disassembly += ", [r" + std::to_string(rn);
    disassembly += ", #0x" + common::hex(immed_5) + "]";

    return disassembly;
}

//STRH|LDRH <Rd>, [<Rn>, #<immed_5>*2]
auto thumbDisassembleLoadStoreHalfword(u16 instruction, u32 address, u16 prev) -> std::string {
    bool l = (instruction >> 11) & 0x1;
    u8 immed_5 = (instruction >> 6) & 0x1F;
    u8 rn = (instruction >> 3) & 0x7;
    u8 rd = instruction & 0x7;

    std::string disassembly;
    disassembly = l ? "ldrh" : "strh";
    disassembly += " r" + std::to_string(rd);
    disassembly += ", [r" + std::to_string(rn);
    disassembly += ", #0x" + common::hex(immed_5 * 2) + "]";

    return disassembly;
}

//STR|LDR <Rd>, [SP, #<immed_8>*4]
auto thumbDisassembleSPRelativeLoadStore(u16 instruction, u32 address, u16 prev) -> std::string {
    bool l = (instruction >> 11) & 0x1;
    u8 rd = (instruction >> 8) & 0x7;
    u8 immed_8 = instruction & 0xFF;

    std::string disassembly;
    disassembly = l ? "ldr" : "str";
    disassembly += " r" + std::to_string(rd);
    disassembly += ", [sp, #0x" + common::hex(immed_8 * 4) + "]";

    return disassembly;
}

//ADD <Rd>, PC|SP, #<immed_8>*4
auto thumbDisassembleLoadAddress(u16 instruction, u32 address, u16 prev) -> std::string {
    bool sp = (instruction >> 11) & 0x1;
    u8 rd = (instruction >> 8) & 0x7;
    u8 immed_8 = instruction & 0xFF;

    std::string disassembly;
    disassembly = "add";
    disassembly += " r" + std::to_string(rd);
    disassembly += ", ";
    disassembly +=  sp ? "sp" : "pc";
    disassembly += ", #0x" + common::hex(immed_8 * 4);

    return disassembly;
}

//ADD SP, #{-}<immed_7>*4
auto thumbDisassembleAdjustSP(u16 instruction, u32 address, u16 prev) -> std::string {
    bool s = bits::get<7, 1>(instruction);
    u8 immed_7 = bits::get<0, 7>(instruction);

    std::string disassembly;
    disassembly = "add sp, ";
    disassembly += "#";
    disassembly += s ? "-" : "";
    disassembly += "0x" + common::hex(immed_7 * 4);

    return disassembly;
}

//PUSH <registers> - If the R bit is set then LR is in the register list
//POP <registers>  - If the R bit is set then PC is in the register list
auto thumbDisassemblePushPopRegisters(u16 instruction, u32 address, u16 prev) -> std::string {
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

    disassembly += r ? l ? ", pc}" : ", lr}" : "}";

    return disassembly;
}

//STMIA|LDMIA <Rn>!, <registers>
auto thumbDisassembleLoadStoreMultiple(u16 instruction, u32 address, u16 prev) -> std::string {
    bool l = (instruction >> 11) & 0x1;
    u8 rn = (instruction >> 8) & 0x7;
    u8 registers = instruction & 0xFF;

    std::string disassembly;
    disassembly = l ? "ldmia" : "stmia";
    disassembly += " r" + std::to_string(rn) + "!";
    disassembly += ", {";

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

//B<cond> #<target_address>
auto thumbDisassembleConditionalBranch(u16 instruction, u32 address, u16 prev) -> std::string {
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
    disassembly += " #";
    disassembly += "0x" + common::hex(address + 4 + immediate);

    return disassembly;
}

//SWI <immed_8>
auto thumbDisassembleSoftwareInterrupt(u16 instruction, u32 address, u16 prev) -> std::string {
    u8 immed_8 = instruction & 0xFF;

    std::string disassembly;
    disassembly = "swi";
    disassembly += " #" + std::to_string(immed_8);

    return disassembly;
}

//B #<target_address>
auto thumbDisassembleUnconditionalBranch(u16 instruction, u32 address, u16 prev) -> std::string {
    s32 immediate = bits::get<0, 11>(instruction);
    immediate = bits::sign_extend<12, s32>(immediate << 1);

    std::string disassembly;
    disassembly = "b";
    disassembly += " #";
    disassembly += "0x" + common::hex(address + 4 + immediate);

    return disassembly;
}

//BL #<target_address>
auto thumbDisassembleLongBranch(u16 instruction, u32 address, u16 prev) -> std::string {
    bool second = bits::get<11, 1>(instruction);

    std::string disassembly;

    if(second) {
        //assert((bits::get<11, 5>(prev)) == 0x1E);

        s32 offset = bits::sign_extend<23, s32>((bits::get<0, 11>(prev) << 12) | (bits::get<0, 11>(instruction) << 1));
        disassembly = "bl #0x" + common::hex(address + 2 + offset);
    } else {
        disassembly = "bl";
    }

    return disassembly;
}

auto thumbDisassembleInvalid(u16 instruction, u32 address, u16 prev) -> std::string {
    return "Invalid Instruction";
}

} //namespace emu