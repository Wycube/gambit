#pragma once

#include "channels/PulseChannel.hpp"
#include "channels/WaveChannel.hpp"
#include "channels/NoiseChannel.hpp"
#include "common/Types.hpp"
#include <chrono>
#include <deque>


namespace emu {

class GBA;

class APU {
public:

    APU(GBA &core);

    void reset();

    auto read(u32 address) -> u8;
    void write(u32 address, u8 value);

    void onTimerOverflow(int timer);
    auto isTimerSelected(int timer) -> bool;

private:

    GBA &m_core;
    EventHandle m_update_event;

    PulseChannel m_pulse1;
    PulseChannel m_pulse2;
    WaveChannel m_wave;
    NoiseChannel m_noise;

    u16 m_sndcnt_l; //NR50, NR51
    u16 m_sndcnt_h;
    u8 m_sndcnt_x;  //NR52
    u16 m_sndbias;

    std::deque<s8> m_fifo_a;
    std::deque<s8> m_fifo_b;
    s8 m_fifo_sample_a, m_fifo_sample_b;

    void step(u64 late);
    void sample(u64 late);
};

} //namespace emu