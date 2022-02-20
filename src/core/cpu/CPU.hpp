#pragma once

#include "core/mem/Bus.hpp"
#include "core/debug/Debugger.hpp"
#include "common/Types.hpp"


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

enum Flag {
    FLAG_NEGATIVE = 1 << 31, 
    FLAG_ZERO = 1 << 30, 
    FLAG_CARRY = 1 << 29, 
    FLAG_OVERFLOW = 1 << 28
};


/* 
 * The CPU is an ARM7TDMI that uses the ARMv4TM architecture and supports the ARM
 * and THUMB instruction sets.
 */
class CPU {
private:

    //Registers
    u32 m_regs[15]; //R0-R14
    u32 m_pc; //R15 / Program Counter
    u32 m_banked_regs[10]; //R13-R14 for privilege modes other than user and system
    u32 m_fiq_regs[5]; //R8-R12 for fiq
    u32 m_cpsr; //Current Program Status Register
    u32 m_spsr; //Saved Program Status Register

    u32 m_pipeline[2]; //Stores the instructions being decoded and executed
    // PrivilegeMode m_mode;
    ExecutionState m_exec;

    //Hardware
    Bus &m_bus;

    auto get_register(u8 reg) -> u32&;
    auto get_reg(u8 reg) -> u32;
    void set_reg(u8 reg, u32 value);
    auto get_flag(Flag flag) -> bool;
    void set_flag(Flag flag, bool set);
    auto passed(u8 condition) -> bool;
    auto privileged() -> bool;

    //Arm instruction handlers
    #include "arm/Handlers.inl"

    //Thumb instruction handlers
    #include "thumb/Handlers.inl"

    void execute_arm(u32 instruction);
    void execute_thumb(u16 instruction);

public:

    CPU(Bus &bus);

    void step();
    void loadPipeline();

    void attachDebugger(dbg::Debugger &debugger);
};

} //namespace emu