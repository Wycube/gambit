#pragma once

#include "emulator/core/Scheduler.hpp"
#include "common/Types.hpp"


namespace emu {

class PulseChannel {
public:

    explicit PulseChannel(Scheduler &scheduler);

    void reset();

    auto read(u32 address) -> u8;
    void write(u32 address, u8 value);

    auto amplitude() -> s8;
    void step();

private:

    void tick(u64 late);
    void restart();
    
    u8 sndcnt_l;  //NR10
    u16 sndcnt_h; //NR11, NR12 / NR23, NR24
    u16 sndcnt_x; //NR13, NR14 / NR23, NR24 
    bool enabled;
    u8 wave_duty_pos;
    u8 current_vol;
    u16 envelope_timer;
    u16 length_timer;
    u16 shadow_freq;
    u16 sweep_timer;

    Scheduler &scheduler;
    EventHandle frequency_event;
};

} //namespace emu