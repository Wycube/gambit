#pragma once

#include "Scheduler.hpp"
#include "common/Types.hpp"


namespace emu {

class GBA;

class Timer final {
public:

    explicit Timer(GBA &core);

    void reset();

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

private:

    auto isTimerRunning(u8 timer) -> bool;
    auto getTimerIntermediateValue(u8 timer, bool running) -> u16;
    void updateTimer(u8 timer, u8 old_tmcnt);
    void startTimer(u8 timer);
    void stopTimer(u8 timer);
    void timerOverflowEvent(u8 timer, u64 late);
    void timerOverflow(u8 timer);
    
    u64 timer_start[4];
    u16 timer_counter[4];
    u16 timer_reload[4];
    u16 tmcnt[4];

    GBA &core;
    EventHandle timer_events[4];
};

} //namespace emu