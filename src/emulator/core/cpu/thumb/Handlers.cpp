#include "emulator/core/cpu/CPU.hpp"
#include "emulator/core/cpu/Names.hpp"
#include "emulator/core/GBA.hpp"
#include "Instruction.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

void CPU::thumbUnimplemented(u16 instruction) {
    ThumbInstruction decoded = thumbDecodeInstruction(instruction, state.pc - 4, core.bus.debugRead16(state.pc - 6));

    LOG_FATAL("Unimplemented THUMB Instruction: (PC:{:08X} Type:{}) {}", state.pc - 4, decoded.type, decoded.disassembly);
}

void CPU::thumbMoveShifted(u16 instruction) {
    const u8 opcode = bits::get<11, 2>(instruction);
    u8 immed_5 = bits::get<6, 5>(instruction);
    const u8 rm = bits::get<3, 3>(instruction);
    const u8 rd = bits::get<0, 3>(instruction);
    const u32 value = getRegister(rm);
    u32 result = 0;
    bool carry = state.cpsr.c;

    if(immed_5 == 0 && opcode != 0) {
        immed_5 = 32;
    }

    switch(opcode) {
        case 0 : result = bits::lsl_c(value, immed_5, carry); break;
        case 1 : result = bits::lsr_c(value, immed_5, carry); break;
        case 2 : result = bits::asr_c(value, immed_5, carry, true); break;
        case 3 : return;
    }

    setRegister(rd, result);

    state.cpsr.n = result >> 31;
    state.cpsr.z = result == 0;
    state.cpsr.c = carry;
}

void CPU::thumbAddSubtract(u16 instruction) {
    const bool i = bits::get_bit<10>(instruction);
    const bool s = bits::get_bit<9>(instruction);
    const u8 rm_immed = bits::get<6, 3>(instruction);
    const u8 rn = bits::get<3, 3>(instruction);
    const u8 rd = bits::get<0, 3>(instruction);
    const u32 op_1 = getRegister(rn);
    const u32 op_2 = i ? rm_immed : getRegister(rm_immed);
    u32 result;
    
    if(s) {
        result = op_1 - op_2;
    } else {
        result = op_1 + op_2;
    }

    setRegister(rd, result);

    state.cpsr.n = result >> 31;
    state.cpsr.z = result == 0;
    state.cpsr.c = s ? op_1 >= op_2 : result < op_1;
    
    bool a = op_1 >> 31;
    bool b = op_2 >> 31;
    bool c = result >> 31;
    state.cpsr.v = a ^ (b ^ !s) && a ^ c;
}

void CPU::thumbProcessImmediate(u16 instruction) {
    const u8 opcode = bits::get<11, 2>(instruction);
    const u8 rd = bits::get<8, 3>(instruction);
    const u8 immed_8 = bits::get<0, 8>(instruction);
    const u32 op_1 = getRegister(rd);
    u32 result = 0;

    switch(opcode) {
        case 0 : result = immed_8; break;        //MOV
        case 1 : result = op_1 - immed_8; break; //CMP
        case 2 : result = op_1 + immed_8; break; //ADD
        case 3 : result = op_1 - immed_8; break; //SUB
    }

    if(opcode != 1) {
        setRegister(rd, result);
    }

    state.cpsr.n = result >> 31;
    state.cpsr.z = result == 0;

    //Set carry and overflow for opcodes other than MOV
    if(opcode != 0) {
        bool subtract = opcode != 2;
        
        if(subtract) {
            state.cpsr.c = op_1 >= immed_8;
        } else {
            state.cpsr.c = result < op_1;
        }

        bool a = op_1 >> 31;
        bool c = result >> 31;
        state.cpsr.v = a ^ !subtract && a ^ c;
    }
}

void CPU::thumbALUOperation(u16 instruction) {
    const u8 opcode = bits::get<6, 4>(instruction);
    const u8 rm = bits::get<3, 3>(instruction);
    const u8 rd = bits::get<0, 3>(instruction);
    u32 op_1 = getRegister(rd);
    const u32 op_2 = getRegister(rm);
    u32 result;
    bool shift_carry = state.cpsr.c;

    switch(opcode) {
        case 0x0 : result = op_1 & op_2; break;  //AND
        case 0x1 : result = op_1 ^ op_2; break;  //XOR
        case 0x2 : result = bits::lsl_c(op_1, op_2 & 0xFF, shift_carry); break; //LSL
        case 0x3 : result = bits::lsr_c(op_1, op_2 & 0xFF, shift_carry); break; //LSR
        case 0x4 : result = bits::asr_c(op_1, op_2 & 0xFF, shift_carry); break; //ASR
        case 0x5 : result = op_1 + op_2 + state.cpsr.c; break;                //ADC
        case 0x6 : result = op_1 - op_2 - !state.cpsr.c; break;               //SBC
        case 0x7 : result = bits::ror_c(op_1, op_2 & 0xFF, shift_carry); break; //ROR
        case 0x8 : result = op_1 & op_2; break;     //TST
        case 0x9 : result = -op_2; op_1 = 0; break; //NEG
        case 0xA : result = op_1 - op_2; break;     //CMP
        case 0xB : result = op_1 + op_2; break;     //CMN
        case 0xC : result = op_1 | op_2; break;     //ORR
        case 0xD : result = op_1 * op_2; break;     //MUL
        case 0xE : result = op_1 & ~op_2; break;    //BIC
        case 0xF : result = ~op_2; break;           //MVN
    }

    //Save result for all opcodes except TST, CMP, and CMN
    if(opcode != 0x8 && opcode != 0xA && opcode != 0xB) {
        setRegister(rd, result);
    }

    //Set Negative and Zero flags appropriately
    state.cpsr.n = result >> 31;
    state.cpsr.z = result == 0;

    //Note: The carry flag gets destroyed with a MUL on ARMv4, 
    //however I don't know how, so I will leave it unchanged.

    //Write to the Carry and Overflow flags
    if(opcode == 0x5 || opcode == 0x6 || (opcode >= 0x9 && opcode <= 0xB)) {
        const bool use_carry = opcode == 0x5 || opcode == 0x6;
        const bool subtract = opcode == 0x6 || opcode == 0x9 || opcode == 0xA;

        if(subtract) {
            state.cpsr.c = (u64)op_1 >= (u64)op_2 + (u64)(use_carry ? !state.cpsr.c : 0);
        } else {
            state.cpsr.c = (u64)op_1 + (u64)op_2 + (use_carry ? state.cpsr.c : 0) > 0xFFFFFFFF;
        }

        const bool a = op_1 >> 31;
        const bool b = op_2 >> 31;
        const bool c = result >> 31;
        state.cpsr.v = a ^ (b ^ !subtract) && a ^ c;
    }

    //Write to the Carry flag for shift opcodes
    if(((op_2 & 0xFF) != 0) && (opcode == 2 || opcode == 3 || opcode == 4 || opcode == 7)) {
        state.cpsr.c = shift_carry;
    }
}

void CPU::thumbHiRegisterOp(u16 instruction) {
    const u8 opcode = bits::get<8, 2>(instruction);
    const u8 rs = bits::get_bit<6>(instruction) << 3 | bits::get<3, 3>(instruction);
    const u8 rd = bits::get_bit<7>(instruction) << 3 | bits::get<0, 3>(instruction);
    const u32 op_1 = getRegister(rd);
    const u32 op_2 = getRegister(rs);
    u32 result = 0;

    switch(opcode) {
        case 0 : result = op_1 + op_2; break; //ADD
        case 1 : result = op_1 - op_2; break; //CMP
        case 2 : result = op_2; break;        //MOV
        //BX
    }

    if(opcode != 1) {
        setRegister(rd, result);

        if(rd == 15) {
            flushPipeline();
        }
    } else {
        state.cpsr.n = result >> 31;
        state.cpsr.z = result == 0;
        state.cpsr.c = op_1 >= op_2;
        state.cpsr.v = (op_1 >> 31) ^ (op_2 >> 31) && (op_1 >> 31) ^ (result >> 31);
    }
}

void CPU::thumbBranchExchange(u16 instruction) {
    const bool link = bits::get_bit<7>(instruction);
    const u8 rm = bits::get<3, 4>(instruction);
    const u32 address = getRegister(rm);

    //Store address of next instruction, plus the thumb-bit (in the lsb), in the Link-Register
    if(link) {
        setRegister(14, getRegister(15) - 1);
    }

    //The lowest bit of the address determines the execution mode (1 = THUMB, 0 = ARM)
    state.cpsr.t = address & 1;
    setRegister(15, address);
    flushPipeline();
}

void CPU::thumbPCRelativeLoad(u16 instruction) {
    const u8 rd = bits::get<8, 3>(instruction);
    const u16 offset = bits::get<0, 8>(instruction) * 4;
    const u32 address = bits::align<u32>(getRegister(15)) + offset;
    
    setRegister(rd, core.bus.read32(address));
}

void CPU::thumbLoadStoreRegister(u16 instruction) {
    const bool l = bits::get_bit<11>(instruction);
    const bool b = bits::get_bit<10>(instruction);
    const u8 rm = bits::get<6, 3>(instruction);
    const u8 rn = bits::get<3, 3>(instruction);
    const u8 rd = bits::get<0, 3>(instruction);
    const u32 address = getRegister(rn) + getRegister(rm);

    if(l) {
        if(b) {
            setRegister(rd, core.bus.read8(address));
        } else {
            setRegister(rd, core.bus.readRotated32(address));
        }
    } else {
        if(b) {
            core.bus.write8(address, getRegister(rd) & 0xFF);
        } else {
            core.bus.write32(address, getRegister(rd));
        }
    }
}

void CPU::thumbLoadStoreSigned(u16 instruction) {
    const u8 opcode = bits::get<10, 2>(instruction);
    const u8 rm = bits::get<6, 3>(instruction);
    const u8 rn = bits::get<3, 3>(instruction);
    const u8 rd = bits::get<0, 3>(instruction);
    const u32 address = getRegister(rn) + getRegister(rm);

    switch(opcode) {
        case 0 : core.bus.write16(address, getRegister(rd)); break;
        case 1 : setRegister(rd, bits::sign_extend<8, u32>(core.bus.read8(address))); break;
        case 2 : setRegister(rd, core.bus.readRotated16(address)); break;
        case 3 : setRegister(rd, (address & 1) ? bits::sign_extend<8, u32>(core.bus.readRotated16(address) & 0xFF) : bits::sign_extend<16, u32>(core.bus.readRotated16(address))); break;
    }
}

void CPU::thumbLoadStoreImmediate(u16 instruction) {
    const bool b = bits::get_bit<12>(instruction);
    const bool l = bits::get_bit<11>(instruction);
    const u8 offset = bits::get<6, 5>(instruction) * (b ? 1 : 4);
    const u8 rn = bits::get<3, 3>(instruction);
    const u8 rd = bits::get<0, 3>(instruction);
    const u32 address = getRegister(rn) + offset;

    if(l) {
        if(b) {
            setRegister(rd, core.bus.read8(address));
        } else {
            setRegister(rd, core.bus.readRotated32(address));
        }
    } else {
        if(b) {
            core.bus.write8(address, getRegister(rd) & 0xFF);
        } else {
            core.bus.write32(address, getRegister(rd));
        }
    }
}

void CPU::thumbLoadStoreHalfword(u16 instruction) {
    const bool l = bits::get_bit<11>(instruction);
    const u8 offset = bits::get<6, 5>(instruction) * 2;
    const u8 rn = bits::get<3, 3>(instruction);
    const u8 rd = bits::get<0, 3>(instruction);
    const u32 address = getRegister(rn) + offset;

    if(l) {
        setRegister(rd, core.bus.readRotated16(address));
    } else {
        core.bus.write16(address, getRegister(rd));
    }
}

void CPU::thumbSPRelativeLoadStore(u16 instruction) {
    const bool l = bits::get_bit<11>(instruction);
    const u8 rd = bits::get<8, 3>(instruction);
    const u16 offset = bits::get<0, 8>(instruction) * 4;
    const u32 address = getRegister(13) + offset;

    if(l) {
        setRegister(rd, core.bus.readRotated32(address));
    } else {
        core.bus.write32(address, getRegister(rd));
    }
}

void CPU::thumbLoadAddress(u16 instruction) {
    const bool sp = bits::get_bit<11>(instruction);
    const u8 rd = bits::get<8, 3>(instruction);
    const u16 offset = bits::get<0, 8>(instruction) << 2;
    const u32 address = sp ? getRegister(13) : bits::align<u32>(getRegister(15));

    setRegister(rd, address + offset);
}

void CPU::thumbAdjustSP(u16 instruction) {
    const bool s = bits::get_bit<7>(instruction);
    const u16 offset = bits::get<0, 7>(instruction) * 4;

    setRegister(13, getRegister(13) + offset * (s ? -1 : 1));
}

void CPU::thumbPushPopRegisters(u16 instruction) {
    const bool l = bits::get_bit<11>(instruction);
    const bool r = bits::get_bit<8>(instruction);
    const u8 registers = bits::get<0, 8>(instruction);

    if(l) {
        u32 address = getRegister(13);

        for(size_t i = 0; i < 8; i++) {
            if(bits::get_bit(registers, i)) {
                setRegister(i, core.bus.read32(address));
                address += 4;
            }
        }

        //Set PC
        if(r) {
            setRegister(15, core.bus.read32(address));
            flushPipeline();
        }

        setRegister(13, getRegister(13) + 4 * (bits::popcount<u16>(registers) + r));
    } else {
        u32 address = getRegister(13) - 4 * (bits::popcount<u16>(registers) + r);

        for(size_t i = 0; i < 8; i++) {
            if(bits::get_bit(registers, i)) {
                core.bus.write32(address, getRegister(i));
                address += 4;
            }
        }

        //Store LR
        if(r) {
            core.bus.write32(address, getRegister(14));
        }

        setRegister(13, getRegister(13) - 4 * (bits::popcount<u16>(registers) + r));
    }
}

void CPU::thumbLoadStoreMultiple(u16 instruction) {
    const bool l = bits::get_bit<11>(instruction);
    const u8 rn = bits::get<8, 3>(instruction);
    const u8 registers = bits::get<0, 8>(instruction);
    u32 address = getRegister(rn);
    u32 writeback = address + 4 * bits::popcount<u16>(registers);

    if(l) {
        for(size_t i = 0; i < 8; i++) {
            if(bits::get_bit(registers, i)) {
                setRegister(i, core.bus.read32(address));
                address += 4;
            }
        }

        if(registers == 0) {
            setRegister(15, core.bus.read32(address));
            flushPipeline();
            writeback = getRegister(rn) + 0x40;
        }

        //No writeback if rn is in the register list
        if(!bits::get_bit(registers, rn)) {
            setRegister(rn, writeback);
        }
    } else {
        bool lowest_set = false;

        for(size_t i = 0; i < 8; i++) {
            if(bits::get_bit(registers, i)) {
                //If rn is in the list and is not the lowest set bit, then the new writeback value is written to memory
                if(i == rn && lowest_set) {
                    core.bus.write32(address, writeback);
                } else {
                    core.bus.write32(address, getRegister(i));
                }

                address += 4;
                lowest_set = true;
            }
        }

        if(registers == 0) {
            core.bus.write32(address, getRegister(15) + 2);
            writeback = getRegister(rn) + 0x40;
        }

        setRegister(rn, writeback);
    }
}

void CPU::thumbConditionalBranch(u16 instruction) {
    const u8 condition = bits::get<8, 4>(instruction);

    if(!passed(condition)) {
        return;
    }
    
    const s32 immediate = bits::sign_extend<9, s32>(bits::get<0, 8>(instruction) << 1);

    state.pc += immediate;
    flushPipeline();
}

void CPU::thumbSoftwareInterrupt(u16 instruction) {
    const u8 comment = bits::get<0, 8>(instruction);
    LOG_TRACE("SWI {}(0x{:02X}) called from THUMB Address: {:08X}", function_names[comment > 0x2B ? 0x2B : comment], comment, state.pc - 4);
    LOG_TRACE("Arguments: r0: {:08X}, r1: {:08X}, r2: {:08X}", getRegister(0), getRegister(1), getRegister(2));

    getSpsr(MODE_SUPERVISOR) = state.cpsr;
    setRegister(14, state.pc - 2, MODE_SUPERVISOR);
    state.cpsr.mode = MODE_SUPERVISOR;
    state.cpsr.t = false;
    state.cpsr.i = true;
    state.pc = 0x8;
    flushPipeline();
}

void CPU::thumbUnconditionalBranch(u16 instruction) {
    const s32 immediate = bits::sign_extend<12, s32>(bits::get<0, 11>(instruction) << 1);

    state.pc += immediate;
    flushPipeline();
}

void CPU::thumbLongBranch(u16 instruction) {
    const bool second = bits::get_bit<11>(instruction);

    if(second) {
        const u32 lr = getRegister(14);
        setRegister(14, (getRegister(15) - 2) | 1);
        state.pc = lr + (bits::get<0, 11>(instruction) << 1);
        flushPipeline();
    } else {
        setRegister(14, getRegister(15) + bits::sign_extend<23, s32>(bits::get<0, 11>(instruction) << 12));
    }
}

void CPU::thumbUndefined(u16 instruction) {
    LOG_TRACE("Undefined THUMB Instruction at Address: {:08X}", state.pc - 4);

    // setRegister(14, getRegister(15) - 4, MODE_UNDEFINED);
    // getSpsr(MODE_UNDEFINED) = state.cpsr;
    // state.cpsr.mode = MODE_UNDEFINED;
    // state.cpsr.i = true;
    // state.pc = 0x4;
    // flushPipeline();
}

} //namespace emu