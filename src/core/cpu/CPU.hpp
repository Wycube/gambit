#pragma once

#include "core/mem/Bus.hpp"
#include "core/debug/Debugger.hpp"
#include "Types.hpp"
#include "common/Types.hpp"


namespace emu {

/* 
 * The CPU is an ARM7TDMI that uses the ARMv4TM architecture which supports the ARM
 * and THUMB instruction sets.
 */
class CPU {
public:

    CPU(Bus &bus);

    void reset();

    void step();
    void flushPipeline();
    
    //Temp
    auto getPC() -> u32 {
        return m_state.pc;
    }

    void attachDebugger(dbg::Debugger &debugger);

private:

    //Registers and other stuff
    CPUState m_state;

    //Hardware
    Bus &m_bus;

    auto get_reg_ref(u8 reg, u8 mode = 0) -> u32&;
    auto get_reg(u8 reg, u8 mode = 0) -> u32;
    void set_reg(u8 reg, u32 value, u8 mode = 0);
    auto get_spsr(u8 mode = 0) -> u32&;

    auto get_flag(Flag flag) -> bool;
    void set_flag(Flag flag, bool set);
    auto passed(u8 condition) -> bool;
    void change_mode(PrivilegeMode mode);
    auto mode_from_bits(u8 mode) -> PrivilegeMode;
    auto privileged() -> bool;

    //Arm and Thumb instruction handler declarations
    #include "arm/Handlers.inl"
    #include "thumb/Handlers.inl"

    void execute_arm(u32 instruction);
    void execute_thumb(u16 instruction);
    void service_interrupt();
};

} //namespace emu