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

enum InterruptSource {
    INT_LCD_VB  = 1 << 0,  //LCD V-Blank
    INT_LCD_HB  = 1 << 1,  //LCD H-Blank
    INT_LCD_VC  = 1 << 2,  //LCD V-Counter Match
    INT_TIM_0   = 1 << 3,  //Timer 0 Overflow
    INT_TIM_1   = 1 << 4,  //Timer 1 Overflow
    INT_TIM_2   = 1 << 5,  //Timer 2 Overflow
    INT_TIM_3   = 1 << 6,  //Timer 3 Overflow
    INT_SERIAL  = 1 << 7,  //Serial Communication
    INT_DMA_0   = 1 << 8,  //DMA 0
    INT_DMA_1   = 1 << 9,  //DMA 1
    INT_DMA_2   = 1 << 10, //DMA 2
    INT_DMA_3   = 1 << 11, //DMA 3
    INT_KEYPAD  = 1 << 12, //Keypad
    INT_GAMEPAK = 1 << 13  //Game Pak (external IRQ source)
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

    //Arm instruction handler declarations
    #include "arm/Handlers.inl"

    //Thumb instruction handler declarations
    #include "thumb/Handlers.inl"

    void execute_arm(u32 instruction);
    void execute_thumb(u16 instruction);
    void service_interrupt();

public:

    CPU(Bus &bus);

    void step();
    void loadPipeline();

    void requestInterrupt(InterruptSource source);

    void attachDebugger(dbg::Debugger &debugger);
};

} //namespace emu