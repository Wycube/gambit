#include "core/cpu/CPU.hpp"
#include "Instruction.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

void CPU::thumbUnimplemented(u16 instruction) {
    ThumbInstruction decoded = thumbDecodeInstruction(instruction, m_pc - 4);

    LOG_ERROR("Unimplemented THUMB Instruction: (PC:{:08X} Type:{}) {}", m_pc - 4, decoded.type, decoded.disassembly);
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

        carry = (get_reg(rm) >> (31 - immed_5)) & 0x1;
        result = get_reg(rm) << immed_5;
    } else if(opcode == 1) {
        u8 shift = immed_5 == 0 ? 32 : immed_5;

        carry = (get_reg(rm) >> shift) & 0x1;
        result = get_reg(rm) >> shift;
    } else if(opcode == 2) {
        u8 shift = immed_5 == 0 ? 32 : immed_5;

        carry = (get_reg(rm) >> shift) & 0x1;
        result = get_reg(rm) >> shift | (get_reg(rm) & ~(1 << 31));
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
    bool i = (instruction >> 10) & 0x1;
    bool s = (instruction >> 9) & 0x1;
    u8 rm_immed = (instruction >> 6) & 0x7;
    u8 rn = (instruction >> 3) & 0x7;
    u8 rd = instruction & 0x7;

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
    u8 opcode = (instruction >> 11) & 0x3;
    u8 rd_rn = (instruction >> 8) & 0x7;
    u8 immed_8 = instruction & 0xFF;

    if(opcode == 0) {
        //MOV
        m_regs[rd_rn] = immed_8;

        //Set Flags
        set_flag(FLAG_NEGATIVE, false);
        set_flag(FLAG_ZERO, immed_8 == 0);
    } else if(opcode == 1) {
        //CMP
        LOG_ERROR("Process Immediate CMP is unimplemented!");
    } else if(opcode == 2) {
        //ADD
        LOG_ERROR("Process Immediate ADD is unimplemented!");
    } else if(opcode == 3) {
        //SUB
        LOG_ERROR("Process Immediate SUB is unimplemented!");
    }
}

void CPU::thumbHiRegisterOp(u16 instruction) {
    u8 opcode = (instruction >> 8) & 0x3;
    u8 rm = (instruction >> 3) & 0xF;
    u8 rd_rn = ((instruction >> 4) & 0x8) | (instruction & 0x7);

    if(opcode == 0) {
        //ADD
        LOG_ERROR("Hi Register Oporation ADD unimplemented!");
    } else if(opcode == 1) {
        //CMP
        LOG_ERROR("Hi Register Operation CMP unimplemented!");
    } else if(opcode == 2) {
        //MOV
        m_regs[rd_rn] = m_regs[rm];

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
        set_reg(15, address & ~2);
        m_exec = EXEC_ARM;
        loadPipeline();
    } else {
        //Halfword align the address
        set_reg(15, address & ~1);
        loadPipeline();
    }
}

void CPU::thumbPCRelativeLoad(u16 instruction) {
    u8 rd = (instruction >> 8) & 0x7;
    u8 immed_8 = instruction & 0xFF;

    u32 address = ((m_pc & ~0x3) << 2) + (immed_8 * 4);
    set_reg(rd, m_bus.read32(address));
}

void CPU::thumbLoadAddress(u16 instruction) {
    bool sp = bits::get<11, 1>(instruction); //(instruction >> 11) & 0x1;
    u8 rd = bits::get<8, 3>(instruction); //(instruction >> 8) & 0x7;
    u8 immed_8 = bits::get<0, 8>(instruction); //instruction & 0xFF;

    set_reg(rd, get_reg(sp ? 13 : 15) + immed_8);
}

void CPU::thumbPushPopRegisters(u16 instruction) {
    bool l = (instruction >> 11) & 0x1;
    bool r = (instruction >> 8) & 0x1;
    u8 registers = instruction & 0xFF;

    if(l) {
        //Pop
        LOG_ERROR("Pop registers not implemented for THUMB yet!");
    } else {
        //Push
        u32 address = m_regs[13] - 4 * (bits::popcount_16(registers) + r);

        for(int i = 0; i < 8; i++) {
            if((registers >> i) & 1) {
                //FIX: Subsequent accesses would be Sequential, right?
                m_bus.write32(address, m_regs[i]); //Should be aligned
                address += 4;
            }
        }

        //Store LR
        if(r) {
            m_bus.write32(address, m_regs[14]);
            address += 4;
        }

        m_regs[13] = m_regs[13] - 4 * (bits::popcount_16(registers) + r);
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

    m_pc += immediate;
    loadPipeline();
}

} //namespace emu