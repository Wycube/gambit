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
    u8 opcode = bits::get<11, 2>(instruction);
    u8 immed_5 = bits::get<6, 5>(instruction);
    u8 rm = bits::get<3, 3>(instruction);
    u8 rd = bits::get<0, 3>(instruction);
    u32 value = get_reg(rm);
    u32 result;
    bool carry;

    if(immed_5 == 0 && opcode != 0) {
        immed_5 = 32;
    }

    switch(opcode) {
        case 0 : result = bits::lsl_c(value, immed_5, carry); break;
        case 1 : result = bits::lsr_c(value, immed_5, carry); break;
        case 2 : result = bits::asr_c(value, immed_5, carry, true); break;
        case 3 : return;
    }

    set_reg(rd, result);

    m_state.cpsr.n = result >> 31;
    m_state.cpsr.z = result == 0;
    m_state.cpsr.c = carry;
}

void CPU::thumbAddSubtract(u16 instruction) {
    bool i = bits::get_bit<10>(instruction);
    bool s = bits::get_bit<9>(instruction);
    u8 rm_immed = bits::get<6, 3>(instruction);
    u8 rn = bits::get<3, 3>(instruction);
    u8 rd = bits::get<0, 3>(instruction);
    u32 op_1 = get_reg(rn);
    u32 op_2 = i ? rm_immed : get_reg(rm_immed);
    u32 result;
    
    if(s) {
        result = op_1 - op_2;
    } else {
        result = op_1 + op_2;
    }

    set_reg(rd, result);

    m_state.cpsr.n = result >> 31;
    m_state.cpsr.z = result == 0;
    m_state.cpsr.c = s ? op_1 >= op_2 : result < op_1;
    
    bool a = op_1 >> 31;
    bool b = op_2 >> 31;
    bool c = result >> 31;
    m_state.cpsr.v = a ^ (b ^ !s) && a ^ c;
}

void CPU::thumbProcessImmediate(u16 instruction) {
    u8 opcode = bits::get<11, 2>(instruction);
    u8 rd = bits::get<8, 3>(instruction);
    u8 immed_8 = bits::get<0, 8>(instruction);
    u32 op_1 = get_reg(rd);
    u32 result = 0;

    switch(opcode) {
        case 0 : result = immed_8; break;        //MOV
        case 1 : result = op_1 - immed_8; break; //CMP
        case 2 : result = op_1 + immed_8; break; //ADD
        case 3 : result = op_1 - immed_8; break; //SUB
    }

    if(opcode != 1) {
        set_reg(rd, result);
    }

    m_state.cpsr.n = result >> 31;
    m_state.cpsr.z = result == 0;

    //Set carry and overflow for opcodes other than MOV
    if(opcode != 0) {
        bool subtract = opcode != 2;
        
        if(subtract) {
            m_state.cpsr.c = op_1 >= immed_8;
        } else {
            m_state.cpsr.c = result < op_1;
        }

        bool a = op_1 >> 31;
        bool b = immed_8 >> 7;
        bool c = result >> 31;
        m_state.cpsr.v = a ^ (b ^ !subtract) && a ^ c;
    }
}

void CPU::thumbALUOperation(u16 instruction) {
    u8 opcode = bits::get<6, 4>(instruction);
    u8 rm = bits::get<3, 3>(instruction);
    u8 rd = bits::get<0, 3>(instruction);
    u32 op_1 = get_reg(rd);
    u32 op_2 = get_reg(rm);
    u32 result;
    bool shift_carry = m_state.cpsr.c;

    switch(opcode) {
        case 0x0 : result = op_1 & op_2; break;  //AND
        case 0x1 : result = op_1 ^ op_2; break;  //XOR
        case 0x2 : result = bits::lsl_c(op_1, op_2 & 0xFF, shift_carry); break; //LSL
        case 0x3 : result = bits::lsr_c(op_1, op_2 & 0xFF, shift_carry); break; //LSR
        case 0x4 : result = bits::asr_c(op_1, op_2 & 0xFF, shift_carry); break; //ASR
        case 0x5 : result = op_1 + op_2 + m_state.cpsr.c; break;                //ADC
        case 0x6 : result = op_1 - op_2 - !m_state.cpsr.c; break;               //SBC
        case 0x7 : result = bits::ror_c(op_1, op_2 & 0xFF, shift_carry); break; //ROR
        case 0x8 : result = op_1 & op_2; break;  //TST
        case 0x9 : result = -op_2; break;        //NEG
        case 0xA : result = op_1 - op_2; break;  //CMP
        case 0xB : result = op_1 + op_2; break;  //CMN
        case 0xC : result = op_1 | op_2; break;  //ORR
        case 0xD : result = op_1 * op_2; break;  //MUL
        case 0xE : result = op_1 & ~op_2; break; //BIC
        case 0xF : result = ~op_2; break;        //MVN
    }

    //Save result for all opcodes except TST, CMP, and CMN
    if(opcode != 0x8 && opcode != 0xA && opcode != 0xB) {
        set_reg(rd, result);
    }

    //Set Negative and Zero flags appropriately
    m_state.cpsr.n = result >> 31;
    m_state.cpsr.z = result == 0;

    //Note: The carry flag gets destroyed with a MUL on ARMv4, 
    //however I don't know how, so I will leave it unchanged.

    //Write to the Carry and Overflow flags
    if(opcode == 0x5 || opcode == 0x6 || (opcode >= 0x9 && opcode <= 0xB)) {
        bool use_carry = opcode == 0x5 || opcode == 0x6;
        bool subtract = opcode == 0x6 || opcode == 0x9 || opcode == 0xA;

        if(subtract) {
            m_state.cpsr.c = (u64)op_1 >= (u64)op_2 + (u64)(use_carry ? !m_state.cpsr.c : 0);
        } else {
            m_state.cpsr.c = result < op_1 + (use_carry ? m_state.cpsr.c : 0);
        }

        bool a = op_1 >> 31;
        bool b = op_2 >> 31;
        bool c = result >> 31;
        m_state.cpsr.v = a ^ (b ^ !subtract) && a ^ c;
    }

    //Write to the Carry flag for shift opcodes
    if(((op_2 & 0xFF) != 0) && (opcode == 2 || opcode == 3 || opcode == 4 || opcode == 7)) {
        m_state.cpsr.c = shift_carry;
    }
}

void CPU::thumbHiRegisterOp(u16 instruction) {
    u8 opcode = bits::get<8, 2>(instruction);
    u8 rs = bits::get_bit<6>(instruction) << 3 | bits::get<3, 3>(instruction);
    u8 rd = bits::get_bit<7>(instruction) << 3 | bits::get<0, 3>(instruction);
    u32 op_1 = get_reg(rd);
    u32 op_2 = get_reg(rs);
    u32 result;

    switch(opcode) {
        case 0 : result = op_1 + op_2; break; //ADD
        case 1 : result = op_1 - op_2; break; //CMP
        case 2 : result = op_2; break;        //MOV
    }

    if(opcode != 1) {
        set_reg(rd, result);

        if(rd == 15) {
            flushPipeline();
        }
    } else {
        m_state.cpsr.n = result >> 31;
        m_state.cpsr.z = result == 0;
        m_state.cpsr.c = op_1 >= op_2;
        m_state.cpsr.v = (op_1 >> 31) ^ !(op_2 >> 31) && (op_1 >> 31) ^ (result >> 31);
    }
}

void CPU::thumbBranchExchange(u16 instruction) {
    bool link = bits::get_bit<7>(instruction);
    u8 rm = bits::get<3, 4>(instruction);
    u32 address = get_reg(rm);

    //Store address of next instruction, plus the thumb-bit (in the lsb), in the Link-Register
    if(link) {
        set_reg(14, get_reg(15) - 1);
    }

    //The lowest bit of the address determines the execution mode (1 = THUMB, 0 = ARM)
    m_state.cpsr.t = address & 1;
    set_reg(15, address);
    flushPipeline();
}

void CPU::thumbPCRelativeLoad(u16 instruction) {
    u8 rd = bits::get<8, 3>(instruction);
    u16 offset = bits::get<0, 8>(instruction) * 4;

    u32 address = bits::align<u32>(get_reg(15)) + offset;
    set_reg(rd, m_bus.read32(address));
}

void CPU::thumbLoadStoreRegister(u16 instruction) {
    bool l = bits::get_bit<11>(instruction);
    bool b = bits::get_bit<10>(instruction);
    u8 rm = bits::get<6, 3>(instruction);
    u8 rn = bits::get<3, 3>(instruction);
    u8 rd = bits::get<0, 3>(instruction);
    u32 address = get_reg(rn) + get_reg(rm);

    if(l) {
        if(b) {
            set_reg(rd, m_bus.read8(address));
        } else {
            set_reg(rd, m_bus.readRotated32(address));
        }
    } else {
        if(b) {
            m_bus.write8(address, get_reg(rd) & 0xFF);
        } else {
            m_bus.write32(address, get_reg(rd));
        }
    }
}

void CPU::thumbLoadStoreSigned(u16 instruction) {
    u8 opcode = bits::get<10, 2>(instruction);
    u8 rm = bits::get<6, 3>(instruction);
    u8 rn = bits::get<3, 3>(instruction);
    u8 rd = bits::get<0, 3>(instruction);
    u32 address = get_reg(rn) + get_reg(rm);

    switch(opcode) {
        case 0 : m_bus.write16(address, get_reg(rd)); break;
        case 1 : set_reg(rd, bits::sign_extend<8, u32>(m_bus.read8(address))); break;
        case 2 : set_reg(rd, m_bus.readRotated16(address)); break;
        case 3 : set_reg(rd, (address & 1) ? bits::sign_extend<8, u32>(m_bus.readRotated16(address) & 0xFF) : bits::sign_extend<16, u32>(m_bus.readRotated16(address))); break;
    }
}

void CPU::thumbLoadStoreImmediate(u16 instruction) {
    bool b = bits::get_bit<12>(instruction);
    bool l = bits::get_bit<11>(instruction);
    u8 offset = bits::get<6, 5>(instruction);
    offset = b ? offset : offset * 4;
    u8 rn = bits::get<3, 3>(instruction);
    u8 rd = bits::get<0, 3>(instruction);
    u32 address = get_reg(rn) + offset;

    if(l) {
        if(b) {
            set_reg(rd, m_bus.read8(address));
        } else {
            set_reg(rd, m_bus.readRotated32(address));
        }
    } else {
        if(b) {
            m_bus.write8(address, get_reg(rd) & 0xFF);
        } else {
            m_bus.write32(address, get_reg(rd));
        }
    }
}

void CPU::thumbLoadStoreHalfword(u16 instruction) {
    bool l = bits::get_bit<11>(instruction);
    u8 offset = bits::get<6, 5>(instruction) * 2;
    u8 rn = bits::get<3, 3>(instruction);
    u8 rd = bits::get<0, 3>(instruction);
    u32 address = get_reg(rn) + offset;

    if(l) {
        set_reg(rd, m_bus.readRotated16(address));
    } else {
        m_bus.write16(address, get_reg(rd));
    }
}

void CPU::thumbSPRelativeLoadStore(u16 instruction) {
    bool l = bits::get_bit<11>(instruction);
    u8 rd = bits::get<8, 3>(instruction);
    u16 offset = bits::get<0, 8>(instruction) * 4;
    u32 address = get_reg(13) + offset;

    if(l) {
        set_reg(rd, m_bus.readRotated32(address));
    } else {
        m_bus.write32(address, get_reg(rd));
    }
}

void CPU::thumbLoadAddress(u16 instruction) {
    bool sp = bits::get_bit<11>(instruction);
    u8 rd = bits::get<8, 3>(instruction);
    u16 offset = bits::get<0, 8>(instruction) << 2;
    u32 address = sp ? get_reg(13) : bits::align<u32>(get_reg(15));

    set_reg(rd, address + offset);
}

void CPU::thumbAdjustSP(u16 instruction) {
    bool s = bits::get_bit<7>(instruction);
    u16 offset = bits::get<0, 7>(instruction) * 4;

    set_reg(13, get_reg(13) + offset * (s ? -1 : 1));
}

void CPU::thumbPushPopRegisters(u16 instruction) {
    bool l = bits::get_bit<11>(instruction);
    bool r = bits::get_bit<8>(instruction);
    u8 registers = bits::get<0, 8>(instruction);

    if(l) {
        u32 address = get_reg(13);

        for(int i = 0; i < 8; i++) {
            if(bits::get_bit(registers, i)) {
                set_reg(i, m_bus.read32(address));
                address += 4;
            }
        }

        //Set PC
        if(r) {
            set_reg(15, m_bus.read32(address));
            flushPipeline();
            address += 4;
        }

        set_reg(13, get_reg(13) + 4 * (bits::popcount<u16>(registers) + r));
    } else {
        u32 address = get_reg(13) - 4 * (bits::popcount<u16>(registers) + r);

        for(int i = 0; i < 8; i++) {
            if(bits::get_bit(registers, i)) {
                m_bus.write32(address, get_reg(i));
                address += 4;
            }
        }

        //Store LR
        if(r) {
            m_bus.write32(address, get_reg(14));
            address += 4;
        }

        set_reg(13, get_reg(13) - 4 * (bits::popcount<u16>(registers) + r));
    }
}

void CPU::thumbLoadStoreMultiple(u16 instruction) {
    bool l = bits::get_bit<11>(instruction);
    u8 rn = bits::get<8, 3>(instruction);
    u8 registers = bits::get<0, 8>(instruction);
    u32 address = get_reg(rn);
    u32 writeback = address + 4 * bits::popcount<u16>(registers);

    if(l) {
        for(int i = 0; i < 8; i++) {
            if(bits::get_bit(registers, i)) {
                set_reg(i, m_bus.read32(address));
                address += 4;
            }
        }

        if(registers == 0) {
            set_reg(15, m_bus.read32(address));
            flushPipeline();
            writeback = get_reg(rn) + 0x40;
        }

        //No writeback if rn is in the register list
        if(!bits::get_bit(registers, rn)) {
            set_reg(rn, writeback);
        }
    } else {
        bool lowest_set = false;

        for(int i = 0; i < 8; i++) {
            if(bits::get_bit(registers, i)) {
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

        if(registers == 0) {
            m_bus.write32(address, get_reg(15) + 2);
            writeback = get_reg(rn) + 0x40;
        }

        set_reg(rn, writeback);
    }
}

void CPU::thumbConditionalBranch(u16 instruction) {
    u8 condition = bits::get<8, 4>(instruction);

    if(!passed(condition)) {
        return;
    }
    
    s32 immediate = bits::sign_extend<9, s32>(bits::get<0, 8>(instruction) << 1);

    set_reg(15, get_reg(15) + immediate);
    flushPipeline();
}

void CPU::thumbSoftwareInterrupt(u16 instruction) {
    LOG_DEBUG("SWI {} called from THUMB Address: {:08X}", bits::get<0, 8>(instruction), m_state.pc - 4);

    get_spsr(MODE_SUPERVISOR) = m_state.cpsr;
    set_reg(14, get_reg(15) - 2, MODE_SUPERVISOR);
    m_state.cpsr.mode = MODE_SUPERVISOR;
    m_state.cpsr.t = false;
    m_state.cpsr.i = true;
    set_reg(15, 0x00000008);
    flushPipeline();
}

void CPU::thumbUnconditionalBranch(u16 instruction) {
    s32 immediate = bits::sign_extend<12, s32>(bits::get<0, 11>(instruction) << 1);

    set_reg(15, get_reg(15) + immediate);
    flushPipeline();
}

void CPU::thumbLongBranch(u16 instruction) {
    bool second = bits::get_bit<11>(instruction);

    if(second) {
        u32 lr = get_reg(14);
        set_reg(14, (get_reg(15) - 2) | 1);
        set_reg(15, lr + (bits::get<0, 11>(instruction) << 1));
        flushPipeline();
    } else {
        set_reg(14, get_reg(15) + bits::sign_extend<23, s32>(bits::get<0, 11>(instruction) << 12));
    }
}

void CPU::thumbUndefined(u16 instruction) {
    LOG_DEBUG("Undefined THUMB Instruction at Address: {:08X}", m_state.pc - 4);
}

} //namespace emu