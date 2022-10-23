#pragma once

#include "channels/PulseChannel.hpp"
#include "common/Types.hpp"
#include <chrono>
#include <queue>


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

    u16 m_sndcnt_h;

    std::queue<s8> m_fifo_a;
    std::queue<s8> m_fifo_b;
    float m_fifo_sample_a, m_fifo_sample_b;

    void update(u64 current, u32 late);
};

} //namespace emu