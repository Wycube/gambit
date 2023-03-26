#pragma once

#include "channels/PulseChannel.hpp"
#include "channels/WaveChannel.hpp"
#include "channels/NoiseChannel.hpp"
#include "common/Types.hpp"
#include <chrono>
#include <deque>


namespace emu {

class GBA;

class APU final {
public:

    explicit APU(GBA &core);

    void reset();

    auto read(u32 address) -> u8;
    void write(u32 address, u8 value);

    void onTimerOverflow(int timer);
    auto isTimerSelected(int timer) -> bool;

private:

    void step(u64 late);
    void sample(u64 late);
    
    GBA &core;
    EventHandle update_event;

    PulseChannel pulse1;
    PulseChannel pulse2;
    WaveChannel wave;
    NoiseChannel noise;

    u16 sndcnt_l; //NR50, NR51
    u16 sndcnt_h;
    u8 sndcnt_x;  //NR52
    u16 sndbias;

    std::deque<s8> fifo_a;
    std::deque<s8> fifo_b;
    s8 fifo_sample_a, fifo_sample_b;
};

} //namespace emu