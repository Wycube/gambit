#include "Disassembly.hpp"
#include "common/StringUtils.hpp"
#include "common/Bits.hpp"
#include "common/Log.hpp"
#include <fmt/format.h>
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
    thumbDisassembleUndefined
};


//LSL|LSR|ASR <Rd>, <Rm>, #<immed_5>
auto thumbDisassembleMoveShifted(u16 instruction, u32 address, u16 prev) -> std::string {
    static const char *OPCODE_MNEMONICS[] = {
        "lsl", "lsr", "asr"
    };

    const u8 opcode = bits::get<11, 2>(instruction);
    const u8 immed_5 = bits::get<6, 5>(instruction);
    const u8 rm = bits::get<3, 3>(instruction);
    const u8 rd = bits::get<0, 3>(instruction);

    return fmt::format("{} r{}, r{}, #0x{:x}", OPCODE_MNEMONICS[opcode], rd, rm, immed_5 == 0 ? 32 : immed_5);
}

//ADD|SUB <Rd>, <Rn>, #<immed_3>
//ADD|SUB <Rd>, <Rn>, <Rm>
auto thumbDisassembleAddSubtract(u16 instruction, u32 address, u16 prev) -> std::string {
    const bool i = bits::get_bit<10>(instruction);
    const bool s = bits::get_bit<9>(instruction);
    const u8 rm_immed = bits::get<6, 3>(instruction);
    const u8 rn = bits::get<3, 3>(instruction);
    const u8 rd = bits::get<0, 3>(instruction);

    return fmt::format("{} r{}, r{}, {}{}", s ? "sub" : "add", rd, rn, i ? "#" : "r", rm_immed);
}

//MOV|CMP|ADD|SUB <Rd/Rn>, #<immed_8>
auto thumbDisassembleProcessImmediate(u16 instruction, u32 address, u16 prev) -> std::string {
    static const char *OPCODE_MNEMONICS[] = {
        "mov", "cmp", "add", "sub"
    };

    const u8 opcode = bits::get<11, 2>(instruction);
    const u8 rd_rn = bits::get<8, 3>(instruction);
    const u8 immed_8 = bits::get<0, 8>(instruction);

    return fmt::format("{} r{}, #0x{:x}", OPCODE_MNEMONICS[opcode], rd_rn, immed_8);
}

//<opcode> <Rd/Rn>, <Rm>
auto thumbDisassembleALUOperation(u16 instruction, u32 address, u16 prev) -> std::string {
    static const char *OPCODE_MNEMONICS[] = {
        "and", "eor", "lsl", "lsr", "asr", "adc", "sbc", "ror",
        "tst", "neg", "cmp", "cmn", "orr", "mul", "bic", "mvn"
    };

    const u8 opcode = bits::get<6, 4>(instruction);
    const u8 rm = bits::get<3, 3>(instruction);
    const u8 rd_rn = bits::get<0, 3>(instruction);

    return fmt::format("{} r{}, r{}", OPCODE_MNEMONICS[opcode], rd_rn, rm);
}

//ADD|CMP|MOV <Rd/Rn>, <Rs>
auto thumbDisassembleHiRegisterOp(u16 instruction, u32 address, u16 prev) -> std::string {
    static const char *OPCODE_MNEMONICS[] = {
        "add", "cmp", "mov"
    };

    const u8 opcode = bits::get<8, 2>(instruction);
    const u8 rs = bits::get_bit<6>(instruction) << 3 | bits::get<3, 3>(instruction);
    const u8 rd = bits::get_bit<7>(instruction) << 3 | bits::get<0, 3>(instruction);
    assert(opcode != 0x3);

    return fmt::format("{} r{}, r{}", OPCODE_MNEMONICS[opcode], rd, rs);
}

//BX|BLX <Rm>
auto thumbDisassembleBranchExchange(u16 instruction, u32 address, u16 prev) -> std::string {
    const bool link = bits::get_bit<7>(instruction);
    const u8 rm = bits::get<3, 4>(instruction);

    return fmt::format("b{}x r{}", link ? "l" : "", rm);
}

//LDR <Rd>, [PC, #<immed_8>*4]
auto thumbDisassemblePCRelativeLoad(u16 instruction, u32 address, u16 prev) -> std::string {
    const u8 rd = bits::get<8, 3>(instruction);
    const u16 offset = bits::get<0, 8>(instruction) << 2;

    return fmt::format("ldr r{}, [pc, #0x{:x}]", rd, offset);
}

//STR|LDR{B} <Rd>, [<Rn>, <Rm>]
auto thumbDisassembleLoadStoreRegister(u16 instruction, u32 address, u16 prev) -> std::string {
    const bool l = bits::get_bit<11>(instruction);
    const bool b = bits::get_bit<10>(instruction);
    const u8 rm = bits::get<6, 3>(instruction);
    const u8 rn = bits::get<3, 3>(instruction);
    const u8 rd = bits::get<0, 3>(instruction);

    return fmt::format("{}{} r{}, [r{}, r{}]", l ? "ldr" : "str", b ? "b" : "", rd, rn, rm);
}

//STRH|LDRSB|LDRH|LDRSH <Rd>, [<Rn>, <Rm>]
auto thumbDisassembleLoadStoreSigned(u16 instruction, u32 address, u16 prev) -> std::string {
    static const char *MNEMONICS[] = {
        "strh", "ldrsb", "ldrh", "ldrsh"
    };

    const u8 hs = bits::get<10, 2>(instruction);
    const u8 rm = bits::get<6, 3>(instruction);
    const u8 rn = bits::get<3, 3>(instruction);
    const u8 rd = bits::get<0, 3>(instruction);

    return fmt::format("{} r{}, [r{}, r{}]", MNEMONICS[hs], rd, rn, rm);
}

//STR|LDR <Rd>, [<Rn>, #<immed_5>*4]
//STRB|LDRB <Rd>, [<Rn>, #<immed_5>]
auto thumbDisassembleLoadStoreImmediate(u16 instruction, u32 address, u16 prev) -> std::string {
    const bool b = bits::get_bit<12>(instruction);
    const bool l = bits::get_bit<11>(instruction);
    u8 offset = bits::get<6, 5>(instruction);
    const u8 rn = bits::get<3, 3>(instruction);
    const u8 rd = bits::get<0, 3>(instruction);

    if(!b) {
        offset <<= 2;
    }

    return fmt::format("{}{} r{}, [r{}, #0x{:x}]", l ? "ldr" : "str", b ? "b" : "", rd, rn, offset);
}

//STRH|LDRH <Rd>, [<Rn>, #<immed_5>*2]
auto thumbDisassembleLoadStoreHalfword(u16 instruction, u32 address, u16 prev) -> std::string {
    const bool l = bits::get_bit<11>(instruction);
    const u8 offset = bits::get<6, 5>(instruction) << 1;
    const u8 rn = bits::get<3, 3>(instruction);
    const u8 rd = bits::get<0, 3>(instruction);

    return fmt::format("{} r{}, [r{}, #0x{:x}]", l ? "ldrh" : "strh", rd, rn, offset);
}

//STR|LDR <Rd>, [SP, #<immed_8>*4]
auto thumbDisassembleSPRelativeLoadStore(u16 instruction, u32 address, u16 prev) -> std::string {
    const bool l = bits::get_bit<11>(instruction);
    const u8 rd = bits::get<8, 3>(instruction);
    const u16 offset = bits::get<0, 8>(instruction) << 2;

    return fmt::format("{} r{}, [sp, #0x{:x}]", l ? "ldr" : "str", rd, offset);
}

//ADD <Rd>, PC|SP, #<immed_8>*4
auto thumbDisassembleLoadAddress(u16 instruction, u32 address, u16 prev) -> std::string {
    const bool sp = bits::get_bit<11>(instruction);
    const u8 rd = bits::get<8, 3>(instruction);
    const u16 offset = bits::get<0, 8>(instruction) << 2;

    return fmt::format("add r{}, {}, #0x{:x}", rd, sp ? "sp" : "pc", offset);
}

//ADD SP, #{-}<immed_7>*4
auto thumbDisassembleAdjustSP(u16 instruction, u32 address, u16 prev) -> std::string {
    const bool s = bits::get_bit<7>(instruction);
    const u16 offset = bits::get<0, 7>(instruction) << 2;

    return fmt::format("add sp, #{}0x{:x}", s ? "-" : "", offset);
}

//PUSH <registers> - If the R bit is set then LR is in the register list
//POP <registers>  - If the R bit is set then PC is in the register list
auto thumbDisassemblePushPopRegisters(u16 instruction, u32 address, u16 prev) -> std::string {
    const bool l = bits::get_bit<11>(instruction);
    const bool r = bits::get_bit<8>(instruction);
    const u8 registers = bits::get<0, 8>(instruction);
    const std::string last_reg = l ? "pc" : "lr";
    std::string register_list;
    
    //Generate a list of the registers based on the set bits
    for(size_t i = 0; i < 8; i++) {
        if(bits::get_bit(registers, i)) {
            //If there are 1s, or in this case a register in the list before this one, add a comma
            register_list += fmt::format("{}r{}", bits::get(0, i, registers) != 0 ? ", " : "", i);
        }
    }
    
    return fmt::format("{} {{{}{}{}}}", l ? "pop" : "push", register_list, registers != 0 && r ? ", " : "", r ? last_reg : "");
}

//STMIA|LDMIA <Rn>!, <registers>
auto thumbDisassembleLoadStoreMultiple(u16 instruction, u32 address, u16 prev) -> std::string {
    const bool l = bits::get_bit<11>(instruction);
    const u8 rn = bits::get<8, 3>(instruction);
    const u8 registers = bits::get<0, 8>(instruction);
    std::string register_list;

    //Generate a list of the registers based on the set bits
    for(size_t i = 0; i < 8; i++) {
        if(bits::get_bit(registers, i)) {
            //If there are 1s, or in this case a register in the list before this one, add a comma
            register_list += fmt::format("{}r{}", bits::get(0, i, registers) != 0 ? ", " : "", i);
        }
    }
    
    return fmt::format("{}ia r{}!, {{{}}}", l ? "ldm" : "stm", rn, register_list);
}

//B<cond> #<target_address>
auto thumbDisassembleConditionalBranch(u16 instruction, u32 address, u16 prev) -> std::string {
    static const char *CONDITION_CODES[] = {
        "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
        "hi", "ls", "ge", "lt", "gt", "le", "",   "nv"
    };

    const u8 condition = bits::get<8, 4>(instruction);
    const s32 immediate = bits::sign_extend<9, s32>(bits::get<0, 8>(instruction) << 1);

    return fmt::format("b{} #0x{:x}", CONDITION_CODES[condition], address + 4 + immediate);
}

//SWI <immed_8>
auto thumbDisassembleSoftwareInterrupt(u16 instruction, u32 address, u16 prev) -> std::string {
    const u8 comment = bits::get<0, 8>(instruction);

    return fmt::format("swi #{}", comment);
}

//B #<target_address>
auto thumbDisassembleUnconditionalBranch(u16 instruction, u32 address, u16 prev) -> std::string {
    const s32 immediate = bits::sign_extend<12, s32>(bits::get<0, 11>(instruction) << 1);

    return fmt::format("b #0x{:x}", address + 4 + immediate);
}

//BL #<target_address>
auto thumbDisassembleLongBranch(u16 instruction, u32 address, u16 prev) -> std::string {
    const bool second = bits::get_bit<11>(instruction);

    if(second) {
        s32 offset = bits::sign_extend<23, s32>((bits::get<0, 11>(prev) << 12) | (bits::get<0, 11>(instruction) << 1));
        return fmt::format("bl #0x{:x}", address + 2 + offset);
    } else {
        return "bl";
    }
}

auto thumbDisassembleUndefined(u16 instruction, u32 address, u16 prev) -> std::string {
    return "undefined";
}

} //namespace emu