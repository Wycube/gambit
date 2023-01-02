#include "emulator/core/cpu/CPU.hpp"
#include "emulator/core/cpu/Names.hpp"
#include "emulator/core/GBA.hpp"
#include "Instruction.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

void CPU::armUnimplemented(u32 instruction) {
    ArmInstruction decoded = armDecodeInstruction(instruction, state.pc - 8);

    LOG_FATAL("Unimplemented ARM Instruction: (PC:{:08X} Type:{}) {}", state.pc - 8, decoded.type, decoded.disassembly);
}

void CPU::armBranchExchange(u32 instruction) {
    const u32 rn = bits::get<0, 4>(instruction);

    state.cpsr.t = getRegister(rn) & 1;
    setRegister(15, getRegister(rn));
    flushPipeline();
}

void CPU::armPSRTransfer(u32 instruction) {
    const bool r = bits::get_bit<22>(instruction);
    const bool s = bits::get_bit<21>(instruction);
    StatusRegister &psr = r ? getSpsr() : state.cpsr;

    if(s) {
        const bool i = bits::get_bit<25>(instruction);
        const u8 fields = bits::get<16, 4>(instruction);
        u32 operand;

        if(i) {
            u8 shift_imm = bits::get<8, 4>(instruction);
            operand = bits::ror(bits::get<0, 8>(instruction), shift_imm << 1);
        } else {
            operand = getRegister(bits::get<0, 4>(instruction));
        }

        //Control Field (Bits 0-7: Mode, IRQ disable, FIQ disable, Thumb bit cannot be changed)
        if(bits::get<0, 1>(fields) && privileged()) {
            psr.i = bits::get_bit<7>(operand);
            psr.f = bits::get_bit<6>(operand);
            psr.mode = bits::get<0, 5>(operand) | 0x10;
        }
        //Status Field (Bits 8-15: Reserved bits)
        if(bits::get<1, 1>(fields) && privileged()) {
            psr.reserved &= ~0xFF;
            psr.reserved |= bits::get<8, 8>(operand);
        }
        //Extension Field (Bits 16-23: Reserved bits)
        if(bits::get<2, 1>(fields) && privileged()) {
            psr.reserved &= ~0xFF00;
            psr.reserved |= bits::get<16, 8>(operand);
        }
        //Flags Field (Bits 24-31: Negative, Zero, Carry, Overflow, some reserved bits)
        if(bits::get<3, 1>(fields)) {
            psr.n = bits::get_bit<31>(operand);
            psr.z = bits::get_bit<30>(operand);
            psr.c = bits::get_bit<29>(operand);
            psr.v = bits::get_bit<28>(operand);
            psr.reserved &= ~0xF0000;
            psr.reserved |= bits::get<24, 4>(operand);
        }
    } else {
        const u8 rd = bits::get<12, 4>(instruction);
        setRegister(rd, psr.asInt());
    }
}

auto CPU::addressMode1(u32 instruction, bool &carry) -> u32 {
    const bool i = bits::get_bit<25>(instruction);

    if(i) {
        const u8 rotate_imm = bits::get<8, 4>(instruction);
        const u8 immed_8 = bits::get<0, 8>(instruction);
        const u32 result = bits::ror(immed_8, rotate_imm * 2);
        carry = rotate_imm == 0 ? state.cpsr.c : result >> 31;

        return result;
    } else {
        const u8 opcode = bits::get<5, 2>(instruction);
        const bool r = bits::get_bit<4>(instruction);
        const u8 rm = bits::get<0, 4>(instruction);
        u8 shift = r ? getRegister(bits::get<8, 4>(instruction)) & 0xFF : bits::get<7, 5>(instruction);
        u32 operand = getRegister(rm);
        u32 result;

        //Specific case for shift by register
        if(r && rm == 15) {
            operand += 4;
        }

        if(shift == 0) {
            if(r || opcode == 0) {
                return operand;
            }

            shift = 32;
        }

        switch(opcode) {
            case 0 : result = bits::lsl_c(operand, shift, carry); break;
            case 1 : result = bits::lsr_c(operand, shift, carry); break;
            case 2 : result = bits::asr_c(operand, shift, carry, !r); break;
            case 3 : result = shift == 32 && !r ? bits::rrx_c(operand, carry) : bits::ror_c(operand, shift, carry); break;
        }

        return result;
    }
}

void CPU::armDataProcessing(u32 instruction) {
    const u8 opcode = bits::get<21, 4>(instruction);
    const bool s = bits::get_bit<20>(instruction);
    const u8 rn = bits::get<16, 4>(instruction);
    const u8 rd = bits::get<12, 4>(instruction);
    bool carry_out = state.cpsr.c;
    u32 op_1 = getRegister(rn);
    const u32 op_2 = addressMode1(instruction, carry_out);
    u32 result;

    //Special case for PC as rn
    if(rn == 15 && !bits::get_bit<25>(instruction) && bits::get_bit<4>(instruction)) {
        op_1 += 4;
    }

    switch(opcode) {
        case 0x0 : result = op_1 & op_2; break;  //AND
        case 0x1 : result = op_1 ^ op_2; break;  //EOR
        case 0x2 : result = op_1 - op_2; break;  //SUB
        case 0x3 : result = op_2 - op_1; break;  //RSB
        case 0x4 : result = op_1 + op_2; break;  //ADD
        case 0x5 : result = op_1 + op_2 + state.cpsr.c; break;  //ADC
        case 0x6 : result = op_1 - op_2 - !state.cpsr.c; break; //SBC
        case 0x7 : result = op_2 - op_1 - !state.cpsr.c; break; //RSC
        case 0x8 : result = op_1 & op_2; break;  //TST
        case 0x9 : result = op_1 ^ op_2; break;  //TEQ
        case 0xA : result = op_1 - op_2; break;  //CMP
        case 0xB : result = op_1 + op_2; break;  //CMN
        case 0xC : result = op_1 | op_2; break;  //ORR
        case 0xD : result = op_2; break;         //MOV
        case 0xE : result = op_1 & ~op_2; break; //BIC
        case 0xF : result = ~op_2; break;        //MVN
    }

    //TST, TEQ, CMP, and CMN only affect flags
    if(rd != 15 && (opcode < 0x8 || opcode > 0xB)) {
        setRegister(rd, result);
    }

    if(rd == 15) {
        if(s) {
            state.cpsr = getSpsr();
        }

        if(opcode < 0x8 || opcode > 0xB) {
            //Set r15 here so if it is switched into thumb, it will be properly aligned
            setRegister(rd, result);
            flushPipeline();
        }
    }

    if(s && rd != 15) {
        state.cpsr.n = result >> 31;
        state.cpsr.z = result == 0;

        if(opcode < 2 || (opcode > 7 && opcode != 0xA && opcode != 0xB)) {
            state.cpsr.c = carry_out;
        } else {
            const bool use_carry = opcode == 5 || opcode == 6 || opcode == 7;
            const bool subtract = opcode == 2 || opcode == 3 || opcode == 6 || opcode == 7 || opcode == 0xA;
            u32 r_op_1 = op_1;
            u32 r_op_2 = op_2;

            //Reverse opcodes (RSB, RSC)
            if(opcode == 3 || opcode == 7) {
                r_op_1 = op_2;
                r_op_2 = op_1;
            }

            if(subtract) {
                state.cpsr.c = (u64)r_op_1 >= (u64)r_op_2 + (u64)(use_carry ? !state.cpsr.c : 0);
            } else {
                state.cpsr.c = (u64)op_1 + (u64)op_2 + (use_carry ? state.cpsr.c : 0) > 0xFFFFFFFF;
            }

            const bool a = r_op_1 >> 31;
            const bool b = r_op_2 >> 31;
            const bool c = result >> 31;
            state.cpsr.v = a ^ (b ^ !subtract) && a ^ c;
        }
    }
}

void CPU::armMultiply(u32 instruction) {
    const bool a = bits::get_bit<21>(instruction);
    const bool s = bits::get_bit<20>(instruction);
    const u8 rd = bits::get<16, 4>(instruction);
    const u8 rn = bits::get<12, 4>(instruction);
    const u8 rs = bits::get<8, 4>(instruction);
    const u8 rm = bits::get<0, 4>(instruction);
    u32 result;

    if(a) {
        result = getRegister(rm) * getRegister(rs) + getRegister(rn);
    } else {
        result = getRegister(rm) * getRegister(rs);
    }

    setRegister(rd, result);

    //Note: The carry flag is destroyed on ARMv4, not
    //sure how though, so I will leave it unchanged.
    if(s) {
        state.cpsr.n = result >> 31;
        state.cpsr.z = result == 0;
    }
}

void CPU::armMultiplyLong(u32 instruction) {
    const bool sign = bits::get_bit<22>(instruction);
    const bool a = bits::get_bit<21>(instruction);
    const bool s = bits::get_bit<20>(instruction);
    const u8 rd_hi = bits::get<16, 4>(instruction);
    const u8 rd_lo = bits::get<12, 4>(instruction);
    const u8 rs = bits::get<8, 4>(instruction);
    const u8 rm = bits::get<0, 4>(instruction);
    u64 result;

    if(sign) {
        result = bits::sign_extend<32, s64>(getRegister(rm)) * bits::sign_extend<32, s64>(getRegister(rs)) + (a ? ((s64)getRegister(rd_hi) << 32) | (getRegister(rd_lo)) : 0);
    } else {
        result = (u64)getRegister(rm) * (u64)getRegister(rs) + (a ? ((u64)getRegister(rd_hi) << 32) | (getRegister(rd_lo)) : 0);
    }

    setRegister(rd_hi, bits::get<32, 32>(result));
    setRegister(rd_lo, bits::get<0, 32>(result));

    //Note: The carry flag is destroyed on ARMv4, like multiply,
    //and apparently the overflow flag as well.
    if(s) {
        state.cpsr.n = result >> 63;
        state.cpsr.z = result == 0;
    }
}

void CPU::armSingleDataSwap(u32 instruction) {
    const bool b = bits::get_bit<22>(instruction);
    const u8 rn = bits::get<16, 4>(instruction);
    const u8 rd = bits::get<12, 4>(instruction);
    const u8 rm = bits::get<0, 4>(instruction);
    const u32 data_32 = core.bus.readRotated32(getRegister(rn));

    if(b) {
        core.bus.write8(getRegister(rn), getRegister(rm) & 0xFF);
    } else {
        core.bus.write32(getRegister(rn), getRegister(rm));
    }

    setRegister(rd, b ? data_32 & 0xFF : data_32);
}

void CPU::armHalfwordTransfer(u32 instruction) {
    const bool p = bits::get_bit<24>(instruction);
    const bool u = bits::get_bit<23>(instruction);
    const bool i = bits::get_bit<22>(instruction);
    const bool w = bits::get_bit<21>(instruction);
    const bool l = bits::get_bit<20>(instruction);
    const u8 rn = bits::get<16, 4>(instruction);
    const u8 rd = bits::get<12, 4>(instruction);
    const u8 sh = bits::get<5, 2>(instruction);
    u32 address = getRegister(rn);
    u32 offset;
    u32 data;
 
    if(i) {
        offset = bits::get<8, 4>(instruction) << 4 | bits::get<0, 4>(instruction);
    } else {
        offset = getRegister(bits::get<0, 4>(instruction));
    }

    u32 offset_address = u ? address + offset : address - offset;

    if(p) {
        address = offset_address;
    }

    if(sh != 2) {
        data = l ? core.bus.readRotated16(address) : getRegister(rd);
    } else {
        //Should not happen with a store
        data = core.bus.read8(address);
    }

    if(l) {
        u32 extended;
        
        if(sh == 2) {
            extended = bits::sign_extend<8, u32>(data);
        } else if(sh == 3) {
            extended = address & 1 ? bits::sign_extend<8, u32>(data) : bits::sign_extend<16, u32>(data);
        } else {
            extended = data;
        }

        if(!p || w) {
            setRegister(rn, offset_address);
        }

        setRegister(rd, extended);
    } else if(sh == 1) {
        if(!p || w) {
            setRegister(rn, offset_address);
        }

        core.bus.write16(address, data);
    }
}

auto CPU::addressMode2(u16 addr_mode, bool i) -> u32 {
    u32 offset;
    
    if(!i) {
        offset = addr_mode;
    } else {
        u8 shift_imm = bits::get<7, 5>(addr_mode);
        const u8 opcode = bits::get<5, 2>(addr_mode);
        const u32 operand = getRegister(bits::get<0, 4>(addr_mode));

        if(opcode != 0 && shift_imm == 0) {
            shift_imm = 32;
        }
        
        switch(opcode) {
            case 0x0 : offset = bits::lsl(operand, shift_imm); break;
            case 0x1 : offset = bits::lsr(operand, shift_imm); break;
            case 0x2 : offset = bits::asr(operand, shift_imm); break;
            case 0x3 : offset = shift_imm == 32 ? bits::rrx(operand, state.cpsr.c) : bits::ror(operand, shift_imm); break;
        }
    }

    return offset;
}

void CPU::armSingleTransfer(u32 instruction) {
    const bool i = bits::get_bit<25>(instruction);
    const bool p = bits::get_bit<24>(instruction);
    const bool u = bits::get_bit<23>(instruction);
    const bool b = bits::get_bit<22>(instruction);
    const bool w = bits::get_bit<21>(instruction);
    const bool l = bits::get_bit<20>(instruction);
    const u8 rn = bits::get<16, 4>(instruction);
    const u8 rd = bits::get<12, 4>(instruction);
    const u32 offset = addressMode2(instruction & 0xFFF, i);
    u32 address = getRegister(rn);
    u32 offset_address = address;

    if(u) {
        offset_address += offset;
    } else {
        offset_address -= offset;
    }

    if(p) {
        address = offset_address;
    }

    if(l) {
        u32 value;

        if(b) {
            value = core.bus.read8(address);
        } else {
            value = core.bus.readRotated32(address);
        }

        //Writeback is optional with pre-indexed addressing
        if(!p || w) {
            setRegister(rn, offset_address);
        }

        setRegister(rd, value);

        if(rd == 15) {
            flushPipeline();
        }
    } else {
        u32 value = getRegister(rd);

        if(rd == 15) {
            value += 4;
        }

        if(b) {
            core.bus.write8(address, value & 0xFF);
        } else {
            core.bus.write32(address, value);
        }

        if(!p || w) {
            setRegister(rn, offset_address);
        }
    }
}

void CPU::armUndefined(u32 instruction) {
    LOG_TRACE("Undefined ARM Instruction at Address: {:08X}", state.pc - 8);

    // setRegister(14, getRegister(15) - 4, MODE_UNDEFINED);
    // getSpsr(MODE_UNDEFINED) = state.cpsr;
    // state.cpsr.mode = MODE_UNDEFINED;
    // state.cpsr.i = true;
    // state.pc = 0x4;
    // flushPipeline();
}

void CPU::armBlockTransfer(u32 instruction) {
    const u8 pu = bits::get<23, 2>(instruction);
    const bool s = bits::get_bit<22>(instruction);
    const bool w = bits::get_bit<21>(instruction);
    const bool l = bits::get_bit<20>(instruction);
    const u8 rn = bits::get<16, 4>(instruction);
    const u16 registers = bits::get<0, 16>(instruction);
    u32 address = getRegister(rn);
    u32 writeback = getRegister(rn) + (4 * bits::popcount<u16>(registers) * (pu & 1 ? 1 : -1));
    const u8 mode = s && !(l && bits::get<15, 1>(registers)) ? MODE_USER : 0;
    

    if(pu == 0 || pu == 2) {
        address -= 4 * bits::popcount<u16>(registers);
    }
    if(pu == 0 || pu == 3) {
        address += 4;
    }

    if(l) {
        for(size_t i = 0; i < 15; i++) {
            if(bits::get_bit(registers, i)) {
                setRegister(i, core.bus.read32(address), mode);
                address += 4;
            }
        }

        if(registers == 0 || bits::get<15, 1>(registers)) {
            state.pc = core.bus.read32(address) & ~3;
            flushPipeline();

            if(registers && s) {
                state.cpsr = getSpsr();
            }
        }

        //No writeback if rn is in the register list
        bool rn_in_list = bits::get(rn, 1, registers);
        if(w && !rn_in_list) {
            setRegister(rn, writeback);
        }

        //Empty rlist causes 0x40 to be added to the base register
        if(registers == 0) {
            setRegister(rn, getRegister(rn) + 0x40);
        }
    } else {
        bool lowest_set = false;

        for(size_t i = 0; i < 15; i++) {
            if(bits::get_bit(registers, i)) {
                //If rn is in the list and is not the lowest set bit, then the new writeback value is written to memory
                if(i == rn && w && lowest_set) {
                    core.bus.write32(address, writeback);
                } else {
                    core.bus.write32(address, getRegister(i, mode));
                }

                address += 4;
                lowest_set = true;
            }
        }

        if(registers == 0 || bits::get<15, 1>(registers)) {
            if(registers == 0) {
                address = getRegister(rn) + (pu == 0 || pu == 2 ? -0x40 : 0);
                address += (pu == 0 || pu == 3) ? 4 : 0;
            }

            core.bus.write32(address, state.pc + 4);
        }

        if(w && !s) {
            setRegister(rn, writeback);
        }

        //Empty rlist causes 0x40 to be added to the base register
        if(registers == 0) {
            setRegister(rn, getRegister(rn) + (pu == 0 || pu == 2 ? -0x40 : 0x40));
        }
    }
}

void CPU::armBranch(u32 instruction) {
    const bool l = bits::get<24, 1>(instruction);
    //Sign extend 24-bit to 32-bit and multiply by 4 so it is word-aligned.
    const s32 immediate = bits::sign_extend<24, s32>(bits::get<0, 24>(instruction)) << 2;

    //Store next instruction's address in the link register
    if(l) {
        setRegister(14, state.pc - 4);
    }

    state.pc += immediate;
    flushPipeline();
}

void CPU::armSoftwareInterrupt(u32 instruction) {
    const u8 comment = bits::get<16, 8>(instruction);
    LOG_TRACE("SWI {}(0x{:02X}) called from THUMB Address: {:08X}", function_names[comment > 0x2B ? 0x2B : comment], comment, state.pc - 8);
    LOG_TRACE("Arguments: r0: {:08X}, r1: {:08X}, r2: {:08X}", getRegister(0), getRegister(1), getRegister(2));

    setRegister(14, getRegister(15) - 4, MODE_SUPERVISOR);
    getSpsr(MODE_SUPERVISOR) = state.cpsr;
    state.cpsr.mode = MODE_SUPERVISOR;
    state.cpsr.i = true;
    state.pc = 0x8;
    flushPipeline();
}

} //namespace emu