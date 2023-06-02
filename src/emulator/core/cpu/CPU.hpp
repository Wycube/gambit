#pragma once

#include "Types.hpp"
#include "common/Types.hpp"
#include <atomic>


namespace emu {

class GBA;

/* 
 * The CPU is an ARM7TDMI that uses the ARMv4T architecture which supports 
 * the 32-bit ARM and 16-bit THUMB instruction sets.
 */
class CPU final {
public:

    explicit CPU(GBA &core);

    void reset(bool skip_bios = true);
    void halt();
    auto halted() -> bool;
    void checkForInterrupt();
    void step();
    void flushPipeline();

    auto readIO(u32 address) -> u8;
    void writeIO(u32 address, u8 value);
    void requestInterrupt(InterruptSource source);
    
    CPUState state;

private:

    void setupRegisterBanks();
    void execute_arm(u32 instruction);
    void execute_thumb(u16 instruction);
    auto service_interrupt() -> bool;
    
    auto getRegister(u8 reg, u8 mode = 0) -> u32;
    void setRegister(u8 reg, u32 value, u8 mode = 0);
    auto getSpsr(u8 mode = 0) -> StatusRegister&;

    auto passed(u8 condition) const -> bool;
    auto modeFromBits(u8 mode) const -> PrivilegeMode;
    auto privileged() const -> bool;
    
    #include "arm/Handlers.inl"
    #include "thumb/Handlers.inl"
    
    GBA &core;

    u16 int_enable;
    //Make access to IF atomic so requesting an interrupt
    //from another thread (i.e. InputDevice) is safe.
    std::atomic<u16> int_flag;
    bool master_enable;
};

} //namespace emu