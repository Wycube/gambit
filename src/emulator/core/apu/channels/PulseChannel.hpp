#pragma once

#include "emulator/core/Scheduler.hpp"
#include "common/Types.hpp"


namespace emu {

class PulseChannel {
public:

    PulseChannel(Scheduler &scheduler);

    void reset();

    auto read(u32 address) -> u8;
    void write(u32 address, u8 value);

    auto amplitude() -> s8;
    void step();

private:

    u8 m_sndcnt_l;  //NR10
    u16 m_sndcnt_h; //NR11, NR12 / NR23, NR24
    u16 m_sndcnt_x; //NR13, NR14 / NR23, NR24 
    bool m_enabled;
    u8 m_wave_duty_pos;
    u8 m_current_vol;
    u16 m_envelope_timer;
    u16 m_length_timer;
    u16 m_shadow_freq;
    u16 m_sweep_timer;

    Scheduler &m_scheduler;
    EventHandle m_frequency_event;

    void tick(u64 late);
    void restart();
};

} //namespace emu