#pragma once

#include "emulator/core/mem/Bus.hpp"
#include "emulator/core/debug/Debugger.hpp"
#include "Types.hpp"
#include "common/Types.hpp"
#include <array>


namespace emu {

/* 
 * The CPU is an ARM7TDMI that uses the ARMv4T architecture which supports 
 * the 32-bit ARM and 16-bit THUMB instruction sets.
 */
class CPU final {
public:

    CPU(Bus &bus, dbg::Debugger &debug);

    void reset();

    void step();
    void flushPipeline();
    
    //Temp
    auto getPC() const -> u32 {
        return m_state.pc;
    }

    void attachDebugger(dbg::Debugger &debugger);

private:

    //Registers and other stuff
    CPUState m_state;

    //Hardware
    Bus &m_bus;
    dbg::Debugger &m_debug;

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