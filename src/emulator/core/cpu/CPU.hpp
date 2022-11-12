#pragma once

#include "Types.hpp"
#include "common/Types.hpp"
#include <array>


namespace emu {

class GBA;

/* 
 * The CPU is an ARM7TDMI that uses the ARMv4T architecture which supports 
 * the 32-bit ARM and 16-bit THUMB instruction sets.
 */
class CPU final {
public:

    CPU(GBA &core);

    void reset();

    void step();
    void flushPipeline();

    void halt();
    auto halted() -> bool;
    void checkForInterrupt();

private:

    //Registers and other stuff
    CPUState m_state;

    //Hardware
    GBA &m_core;
    u32 m_history[32];
    int m_history_index = 0;

    auto get_reg_ref(u8 reg, u8 mode = 0) -> u32&;
    auto get_reg_banked(u8 reg, u8 mode = 0) -> u32&;
    auto get_reg(u8 reg, u8 mode = 0) -> u32;
    void set_reg(u8 reg, u32 value, u8 mode = 0);
    auto get_spsr(u8 mode = 0) -> StatusRegister&;

    auto passed(u8 condition) const -> bool;
    auto mode_from_bits(u8 mode) const -> PrivilegeMode;
    auto privileged() const -> bool;

    //Arm and Thumb instruction handler declarations
    #include "arm/Handlers.inl"
    #include "thumb/Handlers.inl"

    void execute_arm(u32 instruction);
    void execute_thumb(u16 instruction);
    void service_interrupt();
};

} //namespace emu