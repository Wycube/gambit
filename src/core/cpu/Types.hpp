#pragma once

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

enum Flag : u32 {
    FLAG_NEGATIVE = (u32)1 << 31, 
    FLAG_ZERO     = 1 << 30, 
    FLAG_CARRY    = 1 << 29,
    FLAG_OVERFLOW = 1 << 28, 
    FLAG_IRQ      = 1 << 7,
    FLAG_FIQ      = 1 << 6,
    FLAG_THUMB    = 1 << 5
};

enum InterruptSource : u16 {
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

struct CPUState {
    u32 regs[13];
    u32 banked_regs[12];
    u32 fiq_regs[5];
    u32 pc;
    u32 cpsr;
    u32 spsr[5];

    u32 pipeline[2];
    PrivilegeMode mode;
    ExecutionState exec;
};

} //namespace emu