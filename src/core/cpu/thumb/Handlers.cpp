#include "core/cpu/CPU.hpp"
#include "Instruction.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

void CPU::thumbUnimplemented(u16 instruction) {
    ThumbInstruction decoded = thumbDecodeInstruction(instruction, m_pc - 4);

    LOG_ERROR("Unimplemented THUMB Instruction: (PC:{:08X} Type:{}) {}", m_pc - 4, decoded.type, decoded.disassembly);
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

void CPU::thumbPushPopRegisters(u16 instruction) {
    bool l = (instruction >> 11) & 0x1;
    bool r = (instruction >> 8) & 0x1;
    u8 registers = instruction & 0xFF;

    if(l) {
        //Pop
        LOG_ERROR("Pop registers not implemented for THUMB yet!");
    } else {
        //Push
        u32 address = m_regs[13] - 4 * (common::popcount_16(registers) + r);

        for(int i = 0; i < 8; i++) {
            if((registers >> i) & 1) {
                //FIX: Subsequent accesses would be Sequential, right?
                m_bus.write32(address, m_regs[i], AccessType::NON_SEQUENTIAL); //Should be aligned
                address += 4;
            }
        }

        //Store LR
        if(r) {
            m_bus.write32(address, m_regs[14], AccessType::SEQUENTIAL);
            address += 4;
        }

        m_regs[13] = m_regs[13] - 4 * (common::popcount_16(registers) + r);
    }
}

} //namespace emu