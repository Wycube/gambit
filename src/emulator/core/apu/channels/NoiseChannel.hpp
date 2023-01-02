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

    u16 snd4cnt_l; //NR41, NR42
    u16 snd4cnt_h; //NR43, NR44
    bool enabled;
    u8 current_vol;
    u16 envelope_timer;
    u16 length_timer;
    u16 lfsr;
    bool high;

    Scheduler &scheduler;
    EventHandle frequency_event;

    void tick(u64 late);
    void restart();
};

} //namespace emu