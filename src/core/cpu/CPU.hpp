#pragma once

#include "common/Types.hpp"

#include <string>


namespace emu {

enum PrivilegeMode {
    MODE_USER       = 0x10,
    MODE_FIQ        = 0x11,
    MODE_IRQ        = 0x12,
    MODE_SUPERVISOR = 0x13,
    MODE_ABORT      = 0x17,
    MODE_UNDEFINED  = 0x1B,
    MODE_SYSTEM     = 0x1F
};

enum ExecutionState {
    EXEC_ARM,
    EXEC_THUMB
};


/* The CPU is an ARM7TDMI that uses the ARMv4 architecture and supports the ARM
 * and THUMB instruction sets.
 */
class CPU {
private:

    //Registers
    u32 m_gprs[13]; //R0-R12 General Purpose Registers
    u32 m_sp; //R13 / Stack Pointer (usually)
    u32 m_lr; //R14 / Link Register (usually)
    u32 m_pc; //R15 / Program Counter
    u32 m_banked_regs[10]; //R13-R14 for privilege modes other than user and system
    u32 m_fig_regs[5]; //R8-R12 for fiq
    u32 m_cpsr; //Current Program Status Register
    u32 m_spsr; //Saved Program Status Register

    u32 m_pipeline[2]; //Stores the instructions being decoded and executed

public:

};

} //namespace emu