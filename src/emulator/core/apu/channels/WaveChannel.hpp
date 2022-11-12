#pragma once

#include "emulator/core/Scheduler.hpp"
#include "common/Types.hpp"


namespace emu {

class WaveChannel {
public:

    WaveChannel(Scheduler &scheduler);

    void reset();

    auto read(u32 address) -> u8;
    void write(u32 address, u8 value);

    auto amplitude() -> u8;
    void step();

private:

    u8 m_snd3cnt_l;  //NR30
    u16 m_snd3cnt_h; //NR31, NR32
    u16 m_snd3cnt_x; //NR33, NR34
    u8 m_wave_ram[32];

    bool m_enabled;
    u16 m_length_timer;
    u8 m_wave_pos;

    Scheduler &m_scheduler;
    EventHandle m_sample_event;

    void sample(u64 late);
    void restart();
};

} //namespace emu