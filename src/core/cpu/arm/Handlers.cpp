#include "core/cpu/CPU.hpp"
#include "Instruction.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

void CPU::armUnimplemented(u32 instruction) {
    ArmInstruction decoded = armDecodeInstruction(instruction, m_pc - 8);

    LOG_ERROR("Unimplemented ARM Instruction: (PC:{:08X} Type:{}) {}", m_pc - 8, decoded.type, decoded.disassembly);
}

void CPU::armBranchExchange(u32 instruction) {
    u8 condition = instruction >> 28;

    if(!passed(condition)) {
        return;
    }

    u32 rm = get_reg(instruction & 0xF);

    m_exec = rm & 0x1 ? EXEC_THUMB : EXEC_ARM;
    m_cpsr = (m_cpsr & ~(1 << 5)) | ((rm & 0x1) << 5);
    m_pc = rm & 0xFFFFFFFE;
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

        u32 psr = r ? m_spsr : m_cpsr;

        if((fields & 1) && privileged()) {
            psr &= ~0x7F;
            psr |= operand & 0x7F;
        }
        if(((fields >> 1) & 1) && privileged()) {
            psr &= ~(0x7F << 8);
            psr |= operand & (0x7F << 8);
        }
        if(((fields >> 2) & 1) && privileged()) {
            psr &= ~(0x7F << 16);
            psr |= operand & (0x7F << 16);
        }
        if((fields >> 3) & 1) {
            psr &= ~(0x7F << 24);
            psr |= operand & (0x7F << 24);
        }

        if(r) {
            m_spsr = psr;
        } else {
            m_cpsr = psr;
        }
    } else {
        u8 rd = (instruction >> 12) & 0xF;
        //TODO: When executed in User Mode, spsr is the same as cpsr
        set_reg(rd, r ? m_spsr : m_cpsr);
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
        u32 &rm = get_register(instruction & 0xF);

        if(r) {
            LOG_ERROR("Shift by Register not implemented yet!");
            return std::pair(0, false);
        } else {
            u8 shift_imm = (instruction >> 7) & 0x1F;
            u8 op = (instruction >> 5) & 0x3;
            u32 result;
            bool shifter_carry_out;

            switch(op) {
                case 0 : result = rm << shift_imm; //LSL
                    shifter_carry_out = shift_imm == 0 ? get_flag(FLAG_CARRY) : (rm >> (32 - shift_imm)) & 0x1;
                break;
                case 1 : result = shift_imm == 0 ? 0 : rm >> shift_imm; //LSR
                    shifter_carry_out = shift_imm == 0 ? rm >> 31 : (rm >> (shift_imm - 1)) & 0x1;
                break;
                case 2 : result = shift_imm == 0 ? ~(rm >> 31) + 1 : bits::asr(rm, shift_imm); //ASR
                    shifter_carry_out = shift_imm == 0 ? rm >> 31 : (rm >> (shift_imm - 1)) & 0x1;
                break;
                case 3 : result = shift_imm == 0 ? (get_flag(FLAG_CARRY) << 31) | (rm >> 1) : bits::ror(rm, shift_imm); //RRX and ROR
                    shifter_carry_out = shift_imm == 0 ? rm & 0x1 : (rm >> (shift_imm - 1)) & 0x1;
                break;
            }

            return std::pair(result, shifter_carry_out);
        }
    }
}

void CPU::armDataProcessing(u32 instruction) {
    u8 condition = instruction >> 28;

    if(!passed(condition)) {
        return;
    }

    u8 opcode = (instruction >> 21) & 0xF;
    bool s = (instruction >> 20) & 0x1;
    u32 rn = get_reg((instruction >> 16) & 0xF);
    std::pair shifter_out = addressMode1(instruction);
    u32 shifter_operand = shifter_out.first;
    u32 alu_out;

    //Do the operation with the registers
    switch(opcode) {
        case 0x0 : alu_out = rn & shifter_operand; //AND
        break;
        case 0x1 : alu_out = rn & shifter_operand; //EOR
        break;
        case 0x2 : alu_out = rn - shifter_operand; //SUB
        break;
        case 0x3 : alu_out = shifter_operand - rn; //RSB
        break;
        case 0x4 : alu_out = rn + shifter_operand; //ADD
        break;
        case 0x5 : alu_out = rn + shifter_operand + get_flag(FLAG_CARRY); //ADC
        break;
        case 0x6 : alu_out = rn - shifter_operand - !get_flag(FLAG_CARRY); //SBC
        break;
        case 0x7 : alu_out = shifter_operand - rn - !get_flag(FLAG_CARRY); //RSC
        break;
        case 0x8 : alu_out = rn & shifter_operand; //TST
        break;
        case 0x9 : alu_out = rn ^ shifter_operand; //TEQ
        break;
        case 0xA : alu_out = rn - shifter_operand; //CMP
        break;
        case 0xB : alu_out = rn + shifter_operand; //CMN
        break;
        case 0xC : alu_out = rn | shifter_operand; //ORR
        break;
        case 0xD : alu_out = shifter_operand; //MOV
        break;
        case 0xE : alu_out = rn & ~shifter_operand; //BIC
        break;
        case 0xF : alu_out = ~shifter_operand; //MVN
        break;
    }

    if(opcode < 0x8 || opcode > 0xB) {
        set_reg((instruction >> 12) & 0xF, alu_out);

        if(s && ((instruction >> 12) & 0xF) == 15) {
            m_cpsr = m_spsr;
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
            bool subtract = opcode == 2 || opcode == 3 || opcode == 6 || opcode == 7;
            bool reverse = opcode == 3 || opcode == 7;
            bool carry;

            if(subtract) {
                carry = alu_out > (reverse ? shifter_operand : rn) - (use_carry ? get_flag(FLAG_CARRY) : 0);
            } else {
                carry = alu_out < rn + (use_carry ? get_flag(FLAG_CARRY) : 0);
            }
            set_flag(FLAG_CARRY, carry);

            bool rn_neg = (reverse ? shifter_operand & 0x80000000 : rn & 0x80000000);
            bool alu_neg = alu_out & 0x80000000;
            bool overflow = (subtract ? rn_neg && !alu_neg : !rn_neg && alu_neg);
            set_flag(FLAG_OVERFLOW, overflow);
        }
    }
}

void CPU::armHalfwordTransfer(u32 instruction) {
    u8 condition = bits::get<28, 4>(instruction);

    if(!passed(condition)) {
        return;
    }

    u8 rn = bits::get<16, 4>(instruction); //(instruction >> 16) & 0xF;
    u8 rd = bits::get<12, 4>(instruction); //(instruction >> 12) & 0xF;
    bool p = bits::get<24, 1>(instruction); //(instruction >> 24) & 0x1;
    bool u = bits::get<23, 1>(instruction); //(instruction >> 23) & 0x1;
    bool i = bits::get<22, 4>(instruction); //(instruction >> 22) & 0x1;
    bool w = bits::get<21, 4>(instruction); //(instruction >> 21) & 0x1;
    bool l = bits::get<20, 4>(instruction); //(instruction >> 20) & 0x1;
    u8 sh = bits::get<5, 2>(instruction); //(instruction >> 5) & 0x3;

    bool wback = p || !w;
 
    u32 offset = 0;
    if(i) {
        offset = (bits::get<8, 4>(instruction) << 4) | (bits::get<0, 4>(instruction));
    } else {
        offset = get_reg(bits::get<0, 4>(instruction));
    }

    u32 offset_address = u ? get_reg(rn) + offset : get_reg(rn) - offset;
    u32 address = p ? offset_address : get_reg(rn);
    
    u16 data = 0;
    if(sh != 2) {
        if(bits::get<0, 1>(address) == 0) {
            data = l ? m_bus.read16(address) : get_reg(rd);
        } else {
            //TODO: Figure out what UNPREDICTABLE does here
        }
    } else {
        //Should not happen with a store
        data = m_bus.read8(address);
    }

    if(l && wback) set_reg(rn, offset_address);

    if(l) {
        u32 extended = sh == 2 ? bits::sign_extend8(data) : sh == 3 ? bits::sign_extend16(data) : data;
        set_reg(rd, extended);
    } else {
        m_bus.write16(address, data);
    }

    if(!l && wback) set_reg(rn, offset_address);
}

auto CPU::addressMode2(u8 rn, u16 addr_mode, bool i, bool p, bool u, bool w) -> u32 {
    u32 address;
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

    if(u) {
        address = get_reg(rn) + offset;
    } else {
        address = get_reg(rn) - offset;
    }

    if(p && w) {
        set_reg(rn, address);
    } else if(!p && !w) {
        u32 temp = address;
        address = get_reg(rn);
        set_reg(rn, temp);
    }

    return address;
}

void CPU::armSingleTransfer(u32 instruction) {
    u8 condition = instruction >> 28;

    if(!passed(condition)) {
        return;
    }

    u8 rn = (instruction >> 16) & 0xF;
    u8 rd = (instruction >> 12) & 0xF;
    bool i = (instruction >> 25) & 0x1;
    bool p = (instruction >> 24) & 0x1;
    bool u = (instruction >> 23) & 0x1;
    bool b = (instruction >> 22) & 0x1;
    bool w = (instruction >> 21) & 0x1;
    bool l = (instruction >> 20) & 0x1;
    u32 address = addressMode2(rn, instruction & 0xFFF, i, p, u, w);

    if(l) {
        u32 value;

        if(b) {
            value = m_bus.read8(address);
        } else {
            value = bits::ror(m_bus.read32(address), address & 0x3);
        }

        if(rd == 15) {
            m_pc = value & 0xFFFFFFFC;
            loadPipeline();
        } else {
            set_reg(rd, value);
        }
    } else {
        if(b) {
            m_bus.write8(address, get_reg(rd) & 0xFF);
        } else {
            m_bus.write32(address & ~0x3, get_reg(rd));
        }
    }
}

void CPU::armBranch(u32 instruction) {
    u8 condition =  instruction >> 28;

    if(!passed(condition)) {
        return;
    }

    bool l = (instruction >> 24) & 0x1;
    s32 immediate = instruction & 0xFFFFFF;
    immediate |= (immediate >> 23) & 0x1 ? 0xFF000000 : 0; //Sign extend 24-bit to 32-bit
    immediate <<= 2;

    //Store next instruction's address in the link register
    if(l) {
        set_reg(14, m_pc - 4);
    }

    m_pc += immediate;
    loadPipeline();
}

} //namespace emu