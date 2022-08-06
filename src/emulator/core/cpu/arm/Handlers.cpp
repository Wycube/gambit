#include "emulator/core/cpu/CPU.hpp"
#include "Instruction.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

void CPU::armUnimplemented(u32 instruction) {
    ArmInstruction decoded = armDecodeInstruction(instruction, m_state.pc - 8);

    LOG_FATAL("Unimplemented ARM Instruction: (PC:{:08X} Type:{}) {}", m_state.pc - 8, decoded.type, decoded.disassembly);
}

void CPU::armBranchExchange(u32 instruction) {
    u32 rn = bits::get<0, 4>(instruction);

    m_state.cpsr.t = get_reg(rn) & 1;
    set_reg(15, get_reg(rn));
    flushPipeline();
}

void CPU::armPSRTransfer(u32 instruction) {
    bool r = bits::get_bit<22>(instruction);
    bool s = bits::get_bit<21>(instruction);
    StatusRegister &psr = r ? get_spsr() : m_state.cpsr;

    if(s) {
        bool i = bits::get_bit<25>(instruction);
        u8 fields = bits::get<16, 4>(instruction);
        u32 operand;

        if(i) {
            u8 shift_imm = bits::get<8, 4>(instruction);
            operand = bits::ror(bits::get<0, 8>(instruction), shift_imm << 1);
        } else {
            operand = get_reg(bits::get<0, 4>(instruction));
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
        u8 rd = bits::get<12, 4>(instruction);
        set_reg(rd, psr.asInt());
    }
}

auto CPU::addressMode1(u32 instruction, bool &carry) -> u32 {
    bool i = bits::get_bit<25>(instruction);

    if(i) {
        u8 rotate_imm = bits::get<8, 4>(instruction);
        u8 immed_8 = bits::get<0, 8>(instruction);
        u32 result = bits::ror(immed_8, rotate_imm * 2);
        carry = rotate_imm == 0 ? m_state.cpsr.c : result >> 31;

        return result;
    } else {
        u8 opcode = bits::get<5, 2>(instruction);
        bool r = bits::get_bit<4>(instruction);
        u8 rm = bits::get<0, 4>(instruction);
        u8 shift = r ? get_reg(bits::get<8, 4>(instruction)) & 0xFF : bits::get<7, 5>(instruction);
        u32 operand = get_reg(rm);
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
    u8 opcode = bits::get<21, 4>(instruction);
    bool s = bits::get_bit<20>(instruction);
    u8 rn = bits::get<16, 4>(instruction);
    u8 rd = bits::get<12, 4>(instruction);
    bool carry_out = m_state.cpsr.c;
    u32 op_1 = get_reg(rn);
    u32 op_2 = addressMode1(instruction, carry_out);
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
        case 0x5 : result = op_1 + op_2 + m_state.cpsr.c; break;  //ADC
        case 0x6 : result = op_1 - op_2 - !m_state.cpsr.c; break; //SBC
        case 0x7 : result = op_2 - op_1 - !m_state.cpsr.c; break; //RSC
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
        set_reg(rd, result);
    }

    if(rd == 15) {
        if(s) {
            m_state.cpsr = get_spsr();
        }

        //Set r15 here so if it is switched into thumb, it won't be word-aligned before
        set_reg(rd, result);

        if(opcode < 0x8 || opcode > 0xB) {
            flushPipeline();
        }
    }

    if(s && rd != 15) {
        m_state.cpsr.n = result >> 31;
        m_state.cpsr.z = result == 0;

        if(opcode < 2 || (opcode > 7 && opcode != 0xA && opcode != 0xB)) {
            m_state.cpsr.c = carry_out;
        } else {
            bool use_carry = opcode == 5 || opcode == 6 || opcode == 7;
            bool subtract = opcode == 2 || opcode == 3 || opcode == 6 || opcode == 7 || opcode == 0xA;
            u32 r_op_1 = op_1;
            u32 r_op_2 = op_2;

            //Reserve opcodes (RSB, RSC)
            if(opcode == 3 || opcode == 7) {
                r_op_1 = op_2;
                r_op_2 = op_1;
            }

            if(subtract) {
                m_state.cpsr.c = (u64)r_op_1 >= (u64)r_op_2 + (u64)(use_carry ? !m_state.cpsr.c : 0);
            } else {
                m_state.cpsr.c = (u64)op_1 + (u64)op_2 + (use_carry ? m_state.cpsr.c : 0) > 0xFFFFFFFF;
            }

            //This checks if a and b are equal
            //and not equal to c for additions, and if
            //a and b are not equal and a is equal to c for 
            //subtractions. The original version was this:
            // subtract ? a != b && a != c : a == b && a != c
            bool a = r_op_1 >> 31;
            bool b = r_op_2 >> 31;
            bool c = result >> 31;
            m_state.cpsr.v = a ^ (b ^ !subtract) && a ^ c;
        }
    }
}

void CPU::armMultiply(u32 instruction) {
    bool a = bits::get_bit<21>(instruction);
    bool s = bits::get_bit<20>(instruction);
    u8 rd = bits::get<16, 4>(instruction);
    u8 rn = bits::get<12, 4>(instruction);
    u8 rs = bits::get<8, 4>(instruction);
    u8 rm = bits::get<0, 4>(instruction);
    u32 result;

    if(a) {
        result = get_reg(rm) * get_reg(rs) + get_reg(rn);
    } else {
        result = get_reg(rm) * get_reg(rs);
    }

    set_reg(rd, result);

    //Note: The carry flag is destroyed on ARMv4, not
    //sure how though, so I will leave it unchanged.
    if(s) {
        m_state.cpsr.n = result >> 31;
        m_state.cpsr.z = result == 0;
    }
}

void CPU::armMultiplyLong(u32 instruction) {
    bool sign = bits::get_bit<22>(instruction);
    bool a = bits::get_bit<21>(instruction);
    bool s = bits::get_bit<20>(instruction);
    u8 rd_hi = bits::get<16, 4>(instruction);
    u8 rd_lo = bits::get<12, 4>(instruction);
    u8 rs = bits::get<8, 4>(instruction);
    u8 rm = bits::get<0, 4>(instruction);
    u64 result;

    if(sign) {
        result = bits::sign_extend<32, s64>(get_reg(rm)) * bits::sign_extend<32, s64>(get_reg(rs)) + (a ? ((s64)get_reg(rd_hi) << 32) | (get_reg(rd_lo)) : 0);
    } else {
        result = (u64)get_reg(rm) * (u64)get_reg(rs) + (a ? ((u64)get_reg(rd_hi) << 32) | (get_reg(rd_lo)) : 0);
    }

    set_reg(rd_hi, bits::get<32, 32>(result));
    set_reg(rd_lo, bits::get<0, 32>(result));

    //Note: The carry flag is destroyed on ARMv4, like multiply,
    //and apparently the overflow flag as well.
    if(s) {
        m_state.cpsr.n = result >> 63;
        m_state.cpsr.z = result == 0;
    }
}

void CPU::armSingleDataSwap(u32 instruction) {
    bool b = bits::get_bit<22>(instruction);
    u8 rn = bits::get<16, 4>(instruction);
    u8 rd = bits::get<12, 4>(instruction);
    u8 rm = bits::get<0, 4>(instruction);

    u32 data_32 = m_bus.readRotated32(get_reg(rn));

    if(b) {
        m_bus.write8(get_reg(rn), get_reg(rm) & 0xFF);
    } else {
        m_bus.write32(get_reg(rn), get_reg(rm));
    }

    set_reg(rd, b ? data_32 & 0xFF : data_32);
}

void CPU::armHalfwordTransfer(u32 instruction) {
    bool p = bits::get_bit<24>(instruction);
    bool u = bits::get_bit<23>(instruction);
    bool i = bits::get_bit<22>(instruction);
    bool w = bits::get_bit<21>(instruction);
    bool l = bits::get_bit<20>(instruction);
    u8 rn = bits::get<16, 4>(instruction);
    u8 rd = bits::get<12, 4>(instruction);
    u8 sh = bits::get<5, 2>(instruction);
    u32 address = get_reg(rn);
    u32 offset;
    u32 data;
 
    if(i) {
        offset = bits::get<8, 4>(instruction) << 4 | bits::get<0, 4>(instruction);
    } else {
        offset = get_reg(bits::get<0, 4>(instruction));
    }

    u32 offset_address = u ? address + offset : address - offset;

    if(p) {
        address = offset_address;
    }

    if(sh != 2) {
        data = l ? m_bus.readRotated16(address) : get_reg(rd);
    } else {
        //Should not happen with a store
        data = m_bus.read8(address);
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
            set_reg(rn, offset_address);
        }

        set_reg(rd, extended);
    } else if(sh == 1) {
        if(!p || w) {
            set_reg(rn, offset_address);
        }

        m_bus.write16(address, data);
    }
}

auto CPU::addressMode2(u16 addr_mode, bool i) -> u32 {
    u32 offset;
    
    if(!i) {
        offset = addr_mode;
    } else {
        u8 shift_imm = bits::get<7, 5>(addr_mode);
        u8 opcode = bits::get<5, 2>(addr_mode);
        u32 operand = get_reg(bits::get<0, 4>(addr_mode));

        if(opcode != 0 && shift_imm == 0) {
            shift_imm = 32;
        }
        
        switch(opcode) {
            case 0x0 : offset = bits::lsl(operand, shift_imm); break; //LSL
            case 0x1 : offset = bits::lsr(operand, shift_imm); break; //LSR
            case 0x2 : offset = bits::asr(operand, shift_imm); break; //ASR
            case 0x3 : offset = shift_imm == 32 ? bits::rrx(operand, m_state.cpsr.c) : bits::ror(operand, shift_imm); break; //RRX and ROR
        }
    }

    return offset;
}

void CPU::armSingleTransfer(u32 instruction) {
    bool i = bits::get_bit<25>(instruction);
    bool p = bits::get_bit<24>(instruction);
    bool u = bits::get_bit<23>(instruction);
    bool b = bits::get_bit<22>(instruction);
    bool w = bits::get_bit<21>(instruction);
    bool l = bits::get_bit<20>(instruction);
    u8 rn = bits::get<16, 4>(instruction);
    u8 rd = bits::get<12, 4>(instruction);
    u32 offset = addressMode2(instruction & 0xFFF, i);
    u32 address = get_reg(rn);
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
            value = m_bus.read8(address);
        } else {
            value = m_bus.readRotated32(address);
        }

        //Writeback is optional with pre-indexed addressing
        if(!p || w) {
            set_reg(rn, offset_address);
        }

        set_reg(rd, value);

        if(rd == 15) {
            flushPipeline();
        }
    } else {
        u32 value = get_reg(rd);

        if(rd == 15) {
            value += 4;
        }

        if(b) {
            m_bus.write8(address, value & 0xFF);
        } else {
            m_bus.write32(address, value);
        }

        if(!p || w) {
            set_reg(rn, offset_address);
        }
    }
}

void CPU::armUndefined(u32 instruction) {
    LOG_DEBUG("Undefined ARM Instruction at Address: {:08X}", m_state.pc - 8);

    set_reg(14, get_reg(15) - 4, MODE_UNDEFINED);
    get_spsr(MODE_UNDEFINED) = m_state.cpsr;
    m_state.cpsr.mode = MODE_UNDEFINED;
    m_state.cpsr.i = true;
    set_reg(15, 0x00000004);
    flushPipeline();
}

void CPU::armBlockTransfer(u32 instruction) {
    u8 pu = bits::get<23, 2>(instruction);
    bool s = bits::get_bit<22>(instruction);
    bool w = bits::get_bit<21>(instruction);
    bool l = bits::get_bit<20>(instruction);
    u8 rn = bits::get<16, 4>(instruction);
    u16 registers = bits::get<0, 16>(instruction);
    u32 address = get_reg(rn);
    u32 writeback = get_reg(rn) + (4 * bits::popcount<u16>(registers) * (pu & 1 ? 1 : -1));
    u8 mode = s && !(l && bits::get<15, 1>(registers)) ? MODE_USER : 0;
    

    if(pu == 0 || pu == 2) {
        address -= 4 * bits::popcount<u16>(registers);
    }
    if(pu == 0 || pu == 3) {
        address += 4;
    }

    if(l) {
        for(int i = 0; i < 15; i++) {
            if(bits::get_bit(registers, i)) {
                set_reg(i, m_bus.read32(address), mode);
                address += 4;
            }
        }

        if(registers == 0 || bits::get<15, 1>(registers)) {
            m_state.pc = m_bus.read32(address) & ~3;
            flushPipeline();

            if(registers && s) {
                m_state.cpsr = get_spsr();
            }
        }

        //No writeback if rn is in the register list
        bool rn_in_list = bits::get(rn, 1, registers);
        if(w && !rn_in_list) {
            set_reg(rn, writeback);
        }

        //Empty rlist causes 0x40 to be added to the base register
        if(registers == 0) {
            set_reg(rn, get_reg(rn) + 0x40);
        }
    } else {
        bool lowest_set = false;

        for(int i = 0; i < 15; i++) {
            if(bits::get_bit(registers, i)) {
                //If rn is in the list and is not the lowest set bit, then the new writeback value is written to memory
                if(i == rn && w && lowest_set) {
                    m_bus.write32(address, writeback);
                } else {
                    m_bus.write32(address, get_reg(i, mode));
                }

                address += 4;
                lowest_set = true;
            }
        }

        if(registers == 0 || bits::get<15, 1>(registers)) {
            if(registers == 0) {
                address = get_reg(rn) + (pu == 0 || pu == 2 ? -0x40 : 0);
                address += (pu == 0 || pu == 3) ? 4 : 0;
            }

            m_bus.write32(address, m_state.pc + 4);
        }

        if(w && !s) {
            set_reg(rn, writeback);
        }

        //Empty rlist causes 0x40 to be added to the base register
        if(registers == 0) {
            set_reg(rn, get_reg(rn) + (pu == 0 || pu == 2 ? -0x40 : 0x40));
        }
    }
}

void CPU::armBranch(u32 instruction) {
    bool l = bits::get<24, 1>(instruction);
    //Sign extend 24-bit to 32-bit and multiply by 4 so it is word-aligned.
    s32 immediate = bits::sign_extend<24, s32>(bits::get<0, 24>(instruction)) << 2;

    //Store next instruction's address in the link register
    if(l) {
        set_reg(14, get_reg(15) - 4);
    }

    set_reg(15, get_reg(15) + immediate);
    flushPipeline();
}

void CPU::armSoftwareInterrupt(u32 instruction) {
    LOG_DEBUG("SWI {} called from ARM at Address: {:08X}", bits::get<16, 8>(instruction), m_state.pc - 8);

    set_reg(14, get_reg(15) - 4, MODE_SUPERVISOR);
    get_spsr(MODE_SUPERVISOR) = m_state.cpsr;
    m_state.cpsr.mode = MODE_SUPERVISOR;
    m_state.cpsr.i = true;
    set_reg(15, 0x00000008);
    flushPipeline();
}

} //namespace emu