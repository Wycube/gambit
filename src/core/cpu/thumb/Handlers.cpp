#include "core/cpu/CPU.hpp"
#include "Instruction.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

void CPU::thumbUnimplemented(u16 instruction) {
    ThumbInstruction decoded = thumbDecodeInstruction(instruction, m_pc - 4);

    LOG_ERROR("Unimplemented THUMB Instruction: (PC:{:08X} Type:{}) {}", m_pc - 4, decoded.type, decoded.disassembly);
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