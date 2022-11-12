#pragma once

#include "emulator/core/Scheduler.hpp"
#include "common/Types.hpp"


namespace emu {

class NoiseChannel {
public:

    NoiseChannel(Scheduler &scheduler);

    void reset();

    auto read(u32 address) -> u8;
    void write(u32 address, u8 value);

    auto amplitude() -> s8;
    void step();

private:

    u16 m_snd4cnt_l; //NR41, NR42
    u16 m_snd4cnt_h; //NR43, NR44
    bool m_enabled;
    u8 m_current_vol;
    u16 m_envelope_timer;
    u16 m_length_timer;
    u16 m_lfsr;
    bool m_high;

    Scheduler &m_scheduler;
    EventHandle m_frequency_event;

    void tick(u64 late);
    void restart();
};

} //namespace emu