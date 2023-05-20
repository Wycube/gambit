#pragma once

#include "common/Types.hpp"
#include "common/Bits.hpp"


namespace emu {

enum PrivilegeMode : u8 {
    MODE_USER       = 0x10,
    MODE_FIQ        = 0x11,
    MODE_IRQ        = 0x12,
    MODE_SUPERVISOR = 0x13,
    MODE_ABORT      = 0x17,
    MODE_UNDEFINED  = 0x1B,
    MODE_SYSTEM     = 0x1F
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

struct StatusRegister {
    bool n : 1, z : 1, c : 1, v : 1;
    bool i : 1, f : 1, t : 1;
    u8 mode : 5;
    u32 reserved : 20;

    auto asInt() const -> u32 {
        return (n << 31 | z << 30 | c << 29 | v << 28 | reserved << 8 | i << 7 | f << 6 | t << 5 | mode);
    }

    void fromInt(u32 value) {
        n = bits::get_bit<31>(value);
        z = bits::get_bit<30>(value);
        c = bits::get_bit<29>(value);
        v = bits::get_bit<28>(value);
        reserved = bits::get<8, 20>(value);
        i = bits::get_bit<7>(value);
        f = bits::get_bit<6>(value);
        t = bits::get_bit<5>(value);
        mode = bits::get<0, 5>(value) | 0x10;
    }
};

struct CPUState {
    u32 pipeline[2];
    u32 regs[13];
    u32 banked_regs[12];
    u32 fiq_regs[5];
    u32 pc;
    u32 *banks[6][16];
    StatusRegister cpsr;
    StatusRegister spsr[5];
    bool halted;
};

} //namespace emu