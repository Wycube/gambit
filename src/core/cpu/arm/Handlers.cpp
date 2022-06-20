#include "core/cpu/CPU.hpp"
#include "Instruction.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

void CPU::armUnimplemented(u32 instruction) {
    ArmInstruction decoded = armDecodeInstruction(instruction, m_state.pc - 8);

    LOG_FATAL("Unimplemented ARM Instruction: (PC:{:08X} Type:{}) {}", m_state.pc - 8, decoded.type, decoded.disassembly);
}

void CPU::armBranchExchange(u32 instruction) {
    u8 condition = instruction >> 28;

    if(!passed(condition)) {
        return;
    }

    u32 rm = get_reg(instruction & 0xF);

    m_state.exec = rm & 0x1 ? EXEC_THUMB : EXEC_ARM;
    m_state.cpsr = (m_state.cpsr & ~(1 << 5)) | ((rm & 0x1) << 5);
    m_state.pc = rm & (m_state.exec == EXEC_THUMB ? ~1 : ~3); //Align the address
    loadPipeline();
}

void CPU::armPSRTransfer(u32 instruction) {
    u8 condition = instruction >> 28;
    
    if(!passed(condition)) {
        return;
    }
    
    bool r = (instruction >> 22) & 0x1;
    bool s = (instruction >> 21) & 0x1;
    u8 fields = (instruction >> 16) & 0xF;

    if(s) {
        bool i = (instruction >> 25) & 0x1;
        u32 operand = 0;

        if(i) {
            u8 shift_imm = (instruction >> 8) & 0xF;
            operand = bits::ror(instruction & 0xFF, shift_imm << 1);
        } else {
            operand = get_reg(instruction & 0xF);
        }

        u32 psr = r ? get_spsr() : m_state.cpsr;

        if((fields & 1) && privileged()) {
            u8 old_mode = bits::get<0, 5>(psr);
            psr &= ~0xFF;
            psr |= operand & 0xFF;

            if(!r && bits::get<0, 5>(psr) != old_mode) {
                change_mode(mode_from_bits(bits::get<0, 5>(psr)));
            }
        }
        if(((fields >> 1) & 1) && privileged()) {
            psr &= ~(0xFF << 8);
            psr |= operand & (0xFF << 8);
        }
        if(((fields >> 2) & 1) && privileged()) {
            psr &= ~(0xFF << 16);
            psr |= operand & (0xFF << 16);
        }
        if((fields >> 3) & 1) {
            psr &= ~(0xFF << 24);
            psr |= operand & (0xFF << 24);
        }

        if(r) {
            get_spsr() = psr;
        } else {
            m_state.cpsr = psr;
        }
    } else {
        u8 rd = (instruction >> 12) & 0xF;
        set_reg(rd, r ? get_spsr(0) : m_state.cpsr);
    }
}

auto CPU::addressMode1(u32 instruction) -> std::pair<u32, bool> {
    bool i = (instruction >> 25) & 0x1;

    if(i) {
        u8 rotate_imm = (instruction >> 8) & 0xF;
        u8 immed_8 = instruction & 0xFF;
        u32 result = bits::ror(immed_8, rotate_imm * 2);

        bool shifter_carry_out = rotate_imm == 0 ? get_flag(FLAG_CARRY) : result >> 31;

        return std::pair(result, shifter_carry_out);
    } else {
        bool r = (instruction >> 4) & 0x1;
        u64 rm = get_reg(instruction & 0xF);

        //Specific case for shift by register
        if(r && (instruction & 0xF) == 15) {
            rm += 4;
        }

        u8 op = (instruction >> 5) & 0x3;
        u8 shift_imm = r ? get_reg((instruction >> 8) & 0xF) & 0xFF : (instruction >> 7) & 0x1F;
        u64 result;
        bool shifter_carry_out;

        if(r && shift_imm == 0) {
            return std::pair(rm, get_flag(FLAG_CARRY));
        }

        switch(op) {
            case 0 : result = rm << shift_imm; //LSL
                shifter_carry_out = shift_imm == 0 ? get_flag(FLAG_CARRY) : (rm >> (32 - shift_imm)) & 0x1;
            break;
            case 1 : result = shift_imm == 0 && !r ? 0 : rm >> shift_imm; //LSR
                shifter_carry_out = shift_imm == 0 ? rm >> 31 : (rm >> (shift_imm - 1)) & 0x1;
            break;
            case 2 : result = shift_imm == 0 && !r ? ~(rm >> 31) + 1 : bits::asr(rm, shift_imm); //ASR
                shifter_carry_out = shift_imm == 0 ? rm >> 31 : (rm >> ((shift_imm > 32 ? 32 : shift_imm) - 1)) & 0x1;
            break;
            case 3 : result = shift_imm == 0 && !r ? (get_flag(FLAG_CARRY) << 31) | (rm >> 1) : bits::ror(rm, shift_imm); //RRX and ROR
                shifter_carry_out = shift_imm == 0 ? rm & 0x1 : (result >> 31) & 0x1; //(rm >> (shift_imm - 1)) & 0x1;
            break;
        }

        return std::pair(result & 0xFFFFFFFF, shifter_carry_out);
    }
}

//TODO: Refactor these functions  ^  |
//                                |  v

void CPU::armDataProcessing(u32 instruction) {
    u8 condition = instruction >> 28;

    if(!passed(condition)) {
        return;
    }

    u8 opcode = (instruction >> 21) & 0xF;
    bool s = (instruction >> 20) & 0x1;
    u8 rn = (instruction >> 16) & 0xF;
    u32 operand_1 = get_reg(rn);

    //Special case for PC as rn
    if(rn == 15 && !((instruction >> 25) & 0x1) && (instruction >> 4) & 0x1) {
        operand_1 += 4;
    }

    std::pair shifter_out = addressMode1(instruction);
    u32 shifter_operand = shifter_out.first;
    u32 alu_out;

    //Do the operation with the registers
    switch(opcode) {
        case 0x0 : alu_out = operand_1 & shifter_operand; //AND
        break;
        case 0x1 : alu_out = operand_1 ^ shifter_operand; //EOR
        break;
        case 0x2 : alu_out = operand_1 - shifter_operand; //SUB
        break;
        case 0x3 : alu_out = shifter_operand - operand_1; //RSB
        break;
        case 0x4 : alu_out = operand_1 + shifter_operand; //ADD
        break;
        case 0x5 : alu_out = operand_1 + shifter_operand + get_flag(FLAG_CARRY); //ADC
        break;
        case 0x6 : alu_out = operand_1 - shifter_operand - !get_flag(FLAG_CARRY); //SBC
        break;
        case 0x7 : alu_out = shifter_operand - operand_1 - !get_flag(FLAG_CARRY); //RSC
        break;
        case 0x8 : alu_out = operand_1 & shifter_operand; //TST
        break;
        case 0x9 : alu_out = operand_1 ^ shifter_operand; //TEQ
        break;
        case 0xA : alu_out = operand_1 - shifter_operand; //CMP
        break;
        case 0xB : alu_out = operand_1 + shifter_operand; //CMN
        break;
        case 0xC : alu_out = operand_1 | shifter_operand; //ORR
        break;
        case 0xD : alu_out = shifter_operand; //MOV
        break;
        case 0xE : alu_out = operand_1 & ~shifter_operand; //BIC
        break;
        case 0xF : alu_out = ~shifter_operand; //MVN
        break;
    }

    // if(instruction == 0x13811010) {
    //     printf("Yeet the address is %08X\n", m_state.pc);
    // }

    if(opcode < 0x8 || opcode > 0xB) {
        set_reg((instruction >> 12) & 0xF, alu_out);
    }

    if(((instruction >> 12) & 0xF) == 15) {
        if(s) {
            u8 old_mode = bits::get<0, 5>(m_state.cpsr);
            m_state.cpsr = get_spsr(0);

            if(bits::get<0, 5>(m_state.cpsr) != old_mode) {
                change_mode(mode_from_bits(bits::get<0, 5>(m_state.cpsr)));
            }
        }

        if(opcode < 0x8 || opcode > 0xB) {
            loadPipeline();
        }
    }

    //Writeback
    if(s) {
        set_flag(FLAG_NEGATIVE, alu_out >> 31);
        set_flag(FLAG_ZERO, alu_out == 0);

        if(opcode < 2 || (opcode > 7 && opcode != 0xA && opcode != 0xB)) {
            set_flag(FLAG_CARRY, shifter_out.second);
        } else {
            bool use_carry = opcode == 5 || opcode == 6 || opcode == 7;
            bool subtract = opcode == 2 || opcode == 3 || opcode == 6 || opcode == 7 || opcode == 0xA;
            bool reverse = opcode == 3 || opcode == 7;
            bool carry;

            if(subtract) {
                carry = (u64)(reverse ? shifter_operand : operand_1) >= (u64)(reverse ? operand_1 : shifter_operand) + (u64)(use_carry ? !get_flag(FLAG_CARRY) : 0);
            } else {
                carry = (u64)alu_out < (u64)operand_1 + (u64)(use_carry ? get_flag(FLAG_CARRY) : 0);
            }
            set_flag(FLAG_CARRY, carry);

            bool op_1_neg = (reverse ? shifter_operand & 0x80000000 : operand_1 & 0x80000000);
            bool op_2_neg = (reverse ? operand_1 & 0x80000000 : shifter_operand & 0x80000000);
            bool alu_neg = alu_out & 0x80000000;
            bool overflow = (subtract ? op_1_neg != op_2_neg && op_1_neg != alu_neg : op_1_neg == op_2_neg && op_1_neg != alu_neg);
            set_flag(FLAG_OVERFLOW, overflow);
        }
    }
}

void CPU::armMultiply(u32 instruction) {
    u8 condition = bits::get<28, 4>(instruction);

    if(!passed(condition)) {
        return;
    }

    bool accumulate = bits::get<21, 1>(instruction);
    bool s = bits::get<20, 1>(instruction);
    u8 rd = bits::get<16, 4>(instruction);
    u8 rn = bits::get<12, 4>(instruction);
    u8 rs = bits::get<8, 4>(instruction);
    u8 rm = bits::get<0, 4>(instruction);

    if(accumulate) {
        set_reg(rd, get_reg(rm) * get_reg(rs) + get_reg(rn));
    } else {
        set_reg(rd, get_reg(rm) * get_reg(rs));
    }

    //Note: The carry flag is destroyed on ARMv4, not
    //sure how though, so I will leave it unchanged.
    if(s) {
        set_flag(FLAG_NEGATIVE, get_reg(rd) >> 31);
        set_flag(FLAG_ZERO, get_reg(rd) == 0);
    }
}

void CPU::armMultiplyLong(u32 instruction) {
    u8 condition = bits::get<28, 4>(instruction);

    if(!passed(condition)) {
        return;
    }

    u8 rd_hi = bits::get<16, 4>(instruction);
    u8 rd_lo = bits::get<12, 4>(instruction);
    u8 rs = bits::get<8, 4>(instruction);
    u8 rm = bits::get<0, 4>(instruction);
    bool sign = bits::get<22, 1>(instruction);
    bool accumulate = bits::get<21, 1>(instruction);
    bool s = bits::get<20, 1>(instruction);
    u64 result;

    //TODO: Make sure rd != rm and rd, rm, rn, or rs != r15

    if(sign) {
        result = bits::sign_extend<32, s64>(get_reg(rm)) * bits::sign_extend<32, s64>(get_reg(rs)) + (accumulate ? ((s64)get_reg(rd_hi) << 32) | (get_reg(rd_lo)) : 0);
    } else {
        result = (u64)get_reg(rm) * (u64)get_reg(rs) + (accumulate ? ((u64)get_reg(rd_hi) << 32) | (get_reg(rd_lo)) : 0);
    }

    set_reg(rd_hi, bits::get<32, 32>(result));
    set_reg(rd_lo, bits::get<0, 32>(result));

    if(s) {
        set_flag(FLAG_NEGATIVE, result >> 63);
        set_flag(FLAG_ZERO, result == 0);
    }
}

void CPU::armDataSwap(u32 instruction) {
    u8 condition = instruction >> 28;

    if(!passed(condition)) {
        return;
    }

    bool b = bits::get<22, 1>(instruction);
    u8 rn = bits::get<16, 4>(instruction);
    u8 rd = bits::get<12, 4>(instruction);
    u8 rm = bits::get<0, 4>(instruction);

    u32 data_32 = bits::ror(m_bus.read32(get_reg(rn) & ~3), (get_reg(rn) & 3) * 8);

    b ? m_bus.write8(get_reg(rn), get_reg(rm) & 0xFF) : m_bus.write32(get_reg(rn) & ~3, get_reg(rm));
    set_reg(rd, b ? data_32 & 0xFF : data_32);
}

void CPU::armHalfwordTransfer(u32 instruction) {
    u8 condition = bits::get<28, 4>(instruction);

    if(!passed(condition)) {
        return;
    }

    u8 rn = bits::get<16, 4>(instruction);
    u8 rd = bits::get<12, 4>(instruction);
    bool p = bits::get<24, 1>(instruction);
    bool u = bits::get<23, 1>(instruction);
    bool i = bits::get<22, 1>(instruction);
    bool w = bits::get<21, 1>(instruction);
    bool l = bits::get<20, 1>(instruction);
    u8 sh = bits::get<5, 2>(instruction);
 
    u32 offset = 0;
    if(i) {
        offset = (bits::get<8, 4>(instruction) << 4) | (bits::get<0, 4>(instruction));
    } else {
        offset = get_reg(bits::get<0, 4>(instruction));
    }

    u32 offset_address = u ? get_reg(rn) + offset : get_reg(rn) - offset;
    u32 address = p ? offset_address : get_reg(rn);
    u32 data = 0;

    if(sh != 2) {
        data = l ? bits::ror(m_bus.read16(address & ~1), (address & 1) * 8) : get_reg(rd);
    } else {
        //Should not happen with a store
        data = m_bus.read8(address);
    }

    if(l) {
        u32 extended = sh == 2 ? bits::sign_extend<8, u32>(data) : sh == 3 ? (address & 1) ? bits::sign_extend<8, u32>(data) : bits::sign_extend<16, u32>(data) : data;
        
        if(!p || w) {
            set_reg(rn, offset_address);
        }

        set_reg(rd, extended);
    } else if(sh == 1) {
        m_bus.write16(address & ~1, data);

        if(!p || w) {
            set_reg(rn, offset_address);
        }
    }
}

auto CPU::addressMode2(u16 addr_mode, bool i) -> u32 {
    u32 offset;
    
    if(!i) {
        offset = addr_mode;
    } else {
        u8 shift_imm = addr_mode >> 7;
        u8 shift = (addr_mode >> 5) & 0x3;
        u32 rm = get_reg(addr_mode & 0xF);
        
        switch(shift) {
            case 0x0 : offset = rm << shift_imm; //LSL
            break;
            case 0x1 : offset = shift_imm == 0 ? 0 : rm >> shift_imm; //LSR
            break;
            case 0x2 : offset = shift_imm == 0 ? ~(rm >> 31) + 1 : bits::asr(rm, shift_imm); //ASR
            break;
            case 0x3 : offset = shift_imm == 0 ? (get_flag(FLAG_CARRY) << 31) | (rm >> 1) : bits::ror(rm, shift_imm); //RRX and ROR
            break;
        }
    }

    return offset;
}

void CPU::armSingleTransfer(u32 instruction) {
    u8 condition = instruction >> 28;

    if(!passed(condition)) {
        return;
    }

    u8 rn = bits::get<16, 4>(instruction);
    u8 rd = bits::get<12, 4>(instruction);
    bool i = bits::get<25, 1>(instruction);
    bool p = bits::get<24, 1>(instruction);
    bool u = bits::get<23, 1>(instruction);
    bool b = bits::get<22, 1>(instruction);
    bool w = bits::get<21, 1>(instruction);
    bool l = bits::get<20, 1>(instruction);
    u32 offset = addressMode2(instruction & 0xFFF, i);
    u32 address = get_reg(rn);

    if(p) {
        if(u) {
            address += offset;
        } else {
            address -= offset;
        }
    }

    if(l) {
        u32 value;

        if(b) {
            value = m_bus.read8(address);
        } else {
            value = bits::ror(m_bus.read32(address & ~3), (address & 3) * 8);
        }

        //Writeback is optional with pre-indexed addressing
        if(!p || w) {
            set_reg(rn, get_reg(rn) + (u ? offset : -offset));
        }

        if(rd == 15) {
            m_state.pc = value & ~3;
            loadPipeline();
        } else {
            set_reg(rd, value);
        }
    } else {
        u32 value = rd == 15 ? m_state.pc + 4 : get_reg(rd);

        if(b) {
            m_bus.write8(address, value & 0xFF);
        } else {
            m_bus.write32(address & ~3, value);
        }

        if(!p || w) {
            set_reg(rn, get_reg(rn) + (u ? offset : -offset));
        }
    }
}

//TODO: Handle empty rlist and whatever other weird edge-cases this instruction has
void CPU::armBlockTransfer(u32 instruction) {
    u8 condition = instruction >> 28;

    if(!passed(condition)) {
        return;
    }

    u8 rn = bits::get<16, 4>(instruction);
    u8 pu = bits::get<23, 2>(instruction);
    bool s = bits::get<22, 1>(instruction);
    bool w = bits::get<21, 1>(instruction);
    bool l = bits::get<20, 1>(instruction);
    u16 registers = bits::get<0, 16>(instruction);

    u32 address = get_reg(rn);
    if(pu == 0 || pu == 2) {
        address -= 4 * bits::popcount_16(registers);
    }
    if(pu == 0 || pu == 3) {
        address += 4;
    }
    u32 writeback = get_reg(rn) + (4 * bits::popcount_16(registers) * (pu & 1 ? 1 : -1));
    u8 mode = s && !(l && bits::get<15, 1>(registers)) ? MODE_USER : 0;

    if(l) {
        for(int i = 0; i < 15; i++) {
            if(bits::get(i, 1, registers)) {
                set_reg(i, m_bus.read32(address), mode);
                address += 4;
            }
        }

        if(registers == 0 || bits::get<15, 1>(registers)) {
            m_state.pc = m_bus.read32(address) & ~3;
            loadPipeline();

            if(registers && s) {
                m_state.cpsr = get_spsr();
                change_mode(mode_from_bits(bits::get<0, 5, u8>(m_state.cpsr)));
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
            if(bits::get(i, 1, registers)) {
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
    u8 condition = instruction >> 28;

    if(!passed(condition)) {
        return;
    }

    bool l = bits::get<24, 1>(instruction); //(instruction >> 24) & 0x1;
    // s32 immediate = instruction & 0xFFFFFF;
    // immediate |= (immediate >> 23) & 0x1 ? 0xFF000000 : 0; //Sign extend 24-bit to 32-bit
    s32 immediate = bits::sign_extend<24, s32>(bits::get<0, 24>(instruction));
    immediate <<= 2;

    //Store next instruction's address in the link register
    if(l) {
        set_reg(14, m_state.pc - 4);
    }

    m_state.pc += immediate;
    loadPipeline();
}

void CPU::armSoftwareInterrupt(u32 instruction) {
    u8 condition = instruction >> 28;

    if(!passed(condition)) {
        return;
    }

    get_spsr(MODE_SUPERVISOR) = m_state.cpsr;
    get_reg_ref(14, MODE_SUPERVISOR) = m_state.pc - 4;
    change_mode(MODE_SUPERVISOR);
    set_flag(FLAG_IRQ, true);
    m_state.pc = 0x8;
    loadPipeline();
}

} //namespace emu