#include "core/cpu/CPU.hpp"
#include "Instruction.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

void CPU::thumbUnimplemented(u16 instruction) {
    ThumbInstruction decoded = thumbDecodeInstruction(instruction, m_state.pc - 4, m_bus.debugRead16(m_state.pc - 6));

    LOG_FATAL("Unimplemented THUMB Instruction: (PC:{:08X} Type:{}) {}", m_state.pc - 4, decoded.type, decoded.disassembly);
}

void CPU::thumbMoveShifted(u16 instruction) {
    u8 opcode = (instruction >> 11) & 0x3;
    u8 immed_5 = (instruction >> 6) & 0x1F;
    u8 rm = (instruction >> 3) & 0x7;
    u8 rd = instruction & 0x7;

    u32 result;
    u8 carry;

    if(opcode == 0) {
        if(immed_5 == 0) {
            result = get_reg(rm);
            carry = get_flag(FLAG_CARRY);
        }

        carry = (get_reg(rm) >> (32 - immed_5)) & 0x1;
        result = get_reg(rm) << immed_5;
    } else if(opcode == 1) {
        u8 shift = immed_5 == 0 ? 32 : immed_5;

        carry = (get_reg(rm) >> (shift - 1)) & 0x1;
        result = get_reg(rm) >> shift;
    } else if(opcode == 2) {
        u8 shift = immed_5 == 0 ? 32 : immed_5;

        carry = (get_reg(rm) >> shift) & 0x1;
        result = bits::asr(get_reg(rm), shift);
    } else {
        LOG_FATAL("Opcode not suppose to equal 0b11!");
    }

    set_reg(rd, result);

    set_flag(FLAG_NEGATIVE, result >> 31);
    set_flag(FLAG_ZERO, result == 0);
    set_flag(FLAG_CARRY, carry);
}

//TODO: Make sure overflow flag is being set correctly
void CPU::thumbAddSubtract(u16 instruction) {
    bool i = bits::get<10, 1>(instruction); //(instruction >> 10) & 0x1;
    bool s = bits::get<9, 1>(instruction); //(instruction >> 9) & 0x1;
    u8 rm_immed = bits::get<6, 3>(instruction); //(instruction >> 6) & 0x7;
    u8 rn = bits::get<3, 3>(instruction); //(instruction >> 3) & 0x7;
    u8 rd = bits::get<0, 3>(instruction); //instruction & 0x7;

    u32 op_1 = get_reg(rn);
    u32 op_2 = i ? rm_immed : get_reg(rm_immed);
    u32 result;
    
    if(s) {
        result = op_1 - op_2;

        set_flag(FLAG_CARRY, op_1 >= op_2);
        set_flag(FLAG_OVERFLOW, (result & ~(1 << 31)) > (op_1 & ~(1 << 31)));
    } else {
        result = op_1 + op_2;

        set_flag(FLAG_CARRY, result < op_1);
        set_flag(FLAG_OVERFLOW, (result & ~(1 << 31)) < (op_1 & ~(1 << 31)));
    }

    set_reg(rd, result);

    set_flag(FLAG_NEGATIVE, result >> 31);
    set_flag(FLAG_ZERO, result == 0);
}

void CPU::thumbProcessImmediate(u16 instruction) {
    u8 opcode = bits::get<11, 2>(instruction); //(instruction >> 11) & 0x3;
    u8 rd_rn = bits::get<8, 3>(instruction); //(instruction >> 8) & 0x7;
    u8 immed_8 = bits::get<0, 8>(instruction); //instruction & 0xFF;
    u32 op_1 = get_reg(rd_rn);
    u32 result = 0;

    if(opcode == 0) {
        //MOV
        result = immed_8;
        set_reg(rd_rn, immed_8);
    } else if(opcode == 1) {
        //CMP
        result = op_1 - immed_8;
    } else if(opcode == 2) {
        //ADD
        result = op_1 + immed_8;
        set_reg(rd_rn, result);
    } else if(opcode == 3) {
        //SUB
        result = op_1 - immed_8;
        set_reg(rd_rn, result);
    }

    set_flag(FLAG_NEGATIVE, result >> 31);
    set_flag(FLAG_ZERO, result == 0);

    //Set carry and overflow for opcodes other than MOV
    if(opcode != 0) {
        bool subtract = opcode != 2;
        bool carry;
        
        if(subtract) {
            carry = (u64)op_1 >= (u64)immed_8;
        } else {
            carry = (u64)result < (u64)op_1;
        }
        set_flag(FLAG_CARRY, carry);

        bool op_1_neg = op_1 & 0x80000000;
        bool op_2_neg = immed_8 & 0x80;
        bool alu_neg = result & 0x80000000;
        bool overflow = (subtract ? op_1_neg != op_2_neg && op_1_neg != alu_neg : op_1_neg == op_2_neg && op_1_neg != alu_neg);
        set_flag(FLAG_OVERFLOW, overflow);
    }
}

void CPU::thumbALUOperation(u16 instruction) {
    u8 opcode = bits::get<6, 4>(instruction);
    u8 rm = bits::get<3, 3>(instruction);
    u8 rd_rn = bits::get<0, 3>(instruction);

    u32 op_1 = get_reg(rd_rn);
    u32 op_2 = get_reg(rm);
    u32 result;

    switch(opcode) {
        case 0x0 : result = op_1 & op_2; break; //AND
        case 0x1 : result = op_1 ^ op_2; break; //XOR
        case 0x2 : result = op_1 << (op_2 & 0xFF); break; //LSL
        case 0x3 : result = op_1 >> (op_2 & 0xFF); break; //LSR
        case 0x4 : result = bits::asr(op_1, op_2 & 0xFF); break; //ASR
        case 0x5 : result = op_1 + op_2 + get_flag(FLAG_CARRY); break; //ADC
        case 0x6 : result = op_1 - op_2 - !get_flag(FLAG_CARRY); break; //SBC
        case 0x7 : result = bits::ror(op_1, op_2 & 0xFF); break; //ROR
        case 0x8 : result = op_1 & op_2; break; //TST
        case 0x9 : result = -op_2; break; //NEG
        case 0xA : result = op_1 - op_2; break; //CMP
        case 0xB : result = op_1 + op_2; break; //CMN
        case 0xC : result = op_1 | op_2; break; //ORR
        case 0xD : result = op_1 * op_2; break; //MUL
        case 0xE : result = op_1 & ~op_2; break; //BIC
        case 0xF : result = ~op_2; break; //MVN
    }

    //Save result for all opcodes except TST, CMP, and CMN
    if(opcode != 0x8 && opcode != 0xA && opcode != 0xB) {
        set_reg(rd_rn, result);
    }

    //Note: The carry flag gets destroyed with a MUL on ARMv4, 
    //however I don't know how, so I will leave it unchanged.

    //Write to the Carry and Overflow flags
    if(opcode == 0x5 || opcode == 0x6 || (opcode >= 0x9 && opcode <= 0xB)) {
        bool use_carry = opcode == 0x5 || opcode == 0x6;
        bool subtract = opcode == 0x6 || opcode == 0x9 || opcode == 0xA;
        bool carry;

        if(subtract) {
            carry = (u64)op_1 >= (u64)op_2 + (u64)(use_carry ? !get_flag(FLAG_CARRY) : 0);
        } else {
            carry = (u64)result < (u64)op_1 + (u64)(use_carry ? get_flag(FLAG_CARRY) : 0);
        }
        set_flag(FLAG_CARRY, carry);

        bool op_1_neg = op_1 & 0x80000000;
        bool op_2_neg = op_2 & 0x80000000;
        bool alu_neg = result & 0x80000000;
        bool overflow = (subtract ? op_1_neg != op_2_neg && op_1_neg != alu_neg : op_1_neg == op_2_neg && op_1_neg != alu_neg);
        set_flag(FLAG_OVERFLOW, overflow);
    }

    //Write to the Carry flag for shift opcodes
    if(((op_2 & 0xFF) != 0) && ((opcode >= 0x2 && opcode <= 0x4) || opcode == 0x7)) {
        if(opcode == 0x2) set_flag(FLAG_CARRY, (op_1 >> (32 - (op_2 & 0xFF))) & 0x1);
        if(opcode == 0x3) set_flag(FLAG_CARRY, (op_1 >> ((op_2 & 0xFF) - 1)) & 0x1);
        if(opcode == 0x4) set_flag(FLAG_CARRY, (op_1 >> ((op_2 & 0xFF) - 1)) & 0x1);
        if(opcode == 0x7) set_flag(FLAG_CARRY, result >> 31);
    }

    //Set Negative and Zero flags appropriately
    set_flag(FLAG_NEGATIVE, result >> 31);
    set_flag(FLAG_ZERO, result == 0);
}

void CPU::thumbHiRegisterOp(u16 instruction) {
    u8 opcode = (instruction >> 8) & 0x3;
    u8 rm = (instruction >> 3) & 0xF;
    u8 rd_rn = ((instruction >> 4) & 0x8) | (instruction & 0x7);

    if(opcode == 0) {
        //ADD
        set_reg(rd_rn, get_reg(rd_rn) + get_reg(rm));
    
        if(rd_rn == 15) {
            loadPipeline();
        }
    } else if(opcode == 1) {
        //CMP
        u32 result = get_reg(rd_rn) - get_reg(rm);

        set_flag(FLAG_NEGATIVE, result >> 31);
        set_flag(FLAG_ZERO, result == 0);
        set_flag(FLAG_CARRY, get_reg(rd_rn) >= get_reg(rm));
        set_flag(FLAG_OVERFLOW, (result & ~(1 << 31)) > (get_reg(rd_rn) & ~(1 << 31)));
    } else if(opcode == 2) {
        //MOV
        set_reg(rd_rn, get_reg(rm));

        if(rd_rn == 15) {
            loadPipeline();
        }
    }
}

void CPU::thumbBranchExchange(u16 instruction) {
    bool link = bits::get<7, 1>(instruction);
    u8 rm = bits::get<3, 4>(instruction);

    if(link && rm == 15) {
        LOG_FATAL("Using r15 with BLX is not allowed!");
    }

    u32 address = get_reg(rm);

    //Store address of next instruction, plus the thumb-bit (equivilent to +1), in the Link-Register
    if(link) {
        set_reg(14, get_reg(15) - 1);
    }

    if(bits::get<0, 1>(address) == 0) {
        //Word align the address and switch to ARM mode
        m_state.pc = address & ~2;
        m_state.exec = EXEC_ARM;
        set_flag(FLAG_THUMB, false);
        loadPipeline();
    } else {
        //Halfword align the address
        m_state.pc = address & ~1;
        loadPipeline();
    }
}

void CPU::thumbPCRelativeLoad(u16 instruction) {
    u8 rd = (instruction >> 8) & 0x7;
    u8 immed_8 = instruction & 0xFF;

    u32 address = (m_state.pc & ~2) + (immed_8 * 4);
    set_reg(rd, m_bus.read32(address));
}

void CPU::thumbLoadStoreRegister(u16 instruction) {
    bool l = bits::get<11, 1>(instruction); //(instruction >> 11) & 0x1;
    bool b = bits::get<10, 1>(instruction); //(instruction >> 10) & 0x1;
    u8 rm = bits::get<6, 3>(instruction); //(instruction >> 6) & 0x7;
    u8 rn = bits::get<3, 3>(instruction); //(instruction >> 3) & 0x7;
    u8 rd = bits::get<0, 3>(instruction); //instruction & 0x7;
    u32 address = get_reg(rn) + get_reg(rm);

    if(l) {
        set_reg(rd, b ? m_bus.read8(address) : m_bus.read32(address));
    } else {
        if(b) {
            m_bus.write8(address, get_reg(rd) & 0xFF);
        } else {
            m_bus.write32(address, get_reg(rd));
        }
    }
}

void CPU::thumbLoadStoreSigned(u16 instruction) {
    u8 opcode = bits::get<10, 2>(instruction); //(instruction >> 10) & 0x3;
    u8 rm = bits::get<6, 3>(instruction); //(instruction >> 6) & 0x7;
    u8 rn = bits::get<3, 3>(instruction); //(instruction >> 3) & 0x7;
    u8 rd = bits::get<0, 3>(instruction); //instruction & 0x7;

    u32 address = get_reg(rn) + get_reg(rm);

    switch(opcode) {
        case 0 : m_bus.write16(address, get_reg(rd)); break;
        case 1 : set_reg(rd, bits::sign_extend<8, u32>(m_bus.read8(address))); break;
        case 2 : set_reg(rd, m_bus.read16(address)); break;
        case 3 : set_reg(rd, bits::sign_extend<16, u32>(m_bus.read16(address))); break;
    }
}

void CPU::thumbLoadStoreImmediate(u16 instruction) {
    bool b = bits::get<12, 1>(instruction);
    bool l = bits::get<11, 1>(instruction);
    u8 offset = bits::get<6, 5>(instruction);
    offset <<= b ? 0 : 2;
    u8 rn = bits::get<3, 3>(instruction);
    u8 rd = bits::get<0, 3>(instruction);
    u32 address = get_reg(rn) + offset;

    if(l) {
        set_reg(rd, b ? m_bus.read8(address) : m_bus.read32(address));
    } else {
        if(b) {
            m_bus.write8(address, get_reg(rd) & 0xFF);
        } else {
            m_bus.write32(address, get_reg(rd));
        }
    }
}

void CPU::thumbLoadStoreHalfword(u16 instruction) {
    bool l = bits::get<11, 1>(instruction);
    u8 offset = bits::get<6, 5>(instruction) * 2;
    u8 rn = bits::get<3, 3>(instruction);
    u8 rd = bits::get<0, 3>(instruction);

    if(l) {
        set_reg(rd, m_bus.read16(get_reg(rn) + offset));
    } else {
        m_bus.write16(get_reg(rn) + offset, get_reg(rd));
    }
}

void CPU::thumbSPRelativeLoadStore(u16 instruction) {
    bool l = bits::get<11, 1>(instruction);
    u8 rd = bits::get<8, 3>(instruction);
    u16 offset = bits::get<0, 8>(instruction) * 4;

    if(l) {
        set_reg(rd, m_bus.read32(get_reg(13) + offset));
    } else {
        m_bus.write32(get_reg(13) + offset, get_reg(rd));
    }
}

void CPU::thumbLoadAddress(u16 instruction) {
    bool sp = bits::get<11, 1>(instruction);
    u8 rd = bits::get<8, 3>(instruction);
    u8 immed_8 = bits::get<0, 8>(instruction);

    set_reg(rd, get_reg(sp ? 13 : 15) + immed_8);
}

void CPU::thumbAdjustSP(u16 instruction) {
    bool s = bits::get<7, 1>(instruction);
    u16 offset = bits::get<0, 7>(instruction) * 4;

    set_reg(13, get_reg(13) + offset * (s ? -1 : 1));
}

void CPU::thumbPushPopRegisters(u16 instruction) {
    bool l = (instruction >> 11) & 0x1;
    bool r = (instruction >> 8) & 0x1;
    u8 registers = instruction & 0xFF;

    if(l) {
        //Pop
        u32 address = get_reg(13);

        for(int i = 0; i < 8; i++) {
            if(bits::get(i, 1, registers)) {
                set_reg(i, m_bus.read32(address));
                address += 4;
            }
        }

        //Set PC
        if(r) {
            set_reg(15, m_bus.read32(address) & ~1);
            loadPipeline();
            address += 4;
        }

        set_reg(13, get_reg(13) + 4 * (bits::popcount_16(registers) + r));
    } else {
        //Push
        u32 address = get_reg(13) - 4 * (bits::popcount_16(registers) + r);

        for(int i = 0; i < 8; i++) {
            if(bits::get(i, 1, registers)) {
                m_bus.write32(address, get_reg(i));
                address += 4;
            }
        }

        //Store LR
        if(r) {
            m_bus.write32(address, get_reg(14));
            address += 4;
        }

        set_reg(13, get_reg(13) - 4 * (bits::popcount_16(registers) + r));
    }
}

void CPU::thumbLoadStoreMultiple(u16 instruction) {
    bool l = bits::get<11, 1>(instruction); //(instruction >> 11) & 0x1;
    u8 rn = bits::get<8, 3>(instruction); //(instruction >> 8) & 0x7;
    u8 registers = bits::get<0, 8>(instruction); //instruction & 0xFF;
    u32 address = get_reg(rn);
    u32 writeback = get_reg(rn) + 4 * bits::popcount_16(registers);

    if(l) {
        for(int i = 0; i < 8; i++) {
            if(bits::get(i, 1, registers)) {
                set_reg(i, m_bus.read32(address));
                address += 4;
            }
        }

        //No writeback if rn is in the register list
        if(!bits::get(rn, 1, registers)) {
            set_reg(rn, writeback);
        }
    } else {
        bool lowest_set = false;

        for(int i = 0; i < 8; i++) {
            if(bits::get(i, 1, registers)) {
                //If rn is in the list and is not the lowest set bit, then the new writeback value is written to memory
                if(i == rn && lowest_set) {
                    m_bus.write32(address, writeback);
                } else {
                    m_bus.write32(address, get_reg(i));
                }

                address += 4;
                lowest_set = true;
            }
        }

        set_reg(rn, writeback);
    }
}

void CPU::thumbConditionalBranch(u16 instruction) {
    u8 condition = (instruction >> 8) & 0xF;

    //AL (0b1110) is UNDEFINED, TODO: what is the behavior on GBA?
    if(!passed(condition)) {
        return;
    }
    
    s32 immediate = instruction & 0xFF;
    immediate <<= 1;
    immediate |= (immediate >> 8) & 0x1 ? 0xFFFFFF00 : 0; //Sign extend 8-bit to 32-bit

    m_state.pc += immediate;
    loadPipeline();
}

void CPU::thumbSoftwareInterrupt(u16 instruction) {
    get_spsr(MODE_SUPERVISOR) = m_state.cpsr;
    get_reg_ref(14, MODE_SUPERVISOR) = m_state.pc - 2;
    change_mode(MODE_SUPERVISOR);
    set_flag(FLAG_THUMB, false);
    set_flag(FLAG_IRQ, true);
    m_state.exec = EXEC_ARM;
    m_state.pc = 0x8;
    loadPipeline();
}

void CPU::thumbUnconditionalBranch(u16 instruction) {
    s32 immediate = bits::get<0, 11>(instruction);
    immediate = bits::sign_extend<12, s32>(immediate << 1);

    m_state.pc += immediate;
    loadPipeline();
}

void CPU::thumbLongBranch(u16 instruction) {
    bool second = bits::get<11, 1>(instruction);

    if(second) {
        u32 lr = get_reg(14);
        set_reg(14, (get_reg(15) - 2) | 1);
        set_reg(15, lr + (bits::get<0, 11>(instruction) << 1));
        loadPipeline();
    } else {
        set_reg(14, get_reg(15) + bits::sign_extend<23, s32>(bits::get<0, 11>(instruction) << 12));
    }
}

} //namespace emu