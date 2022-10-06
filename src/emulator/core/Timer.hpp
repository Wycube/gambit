#pragma once

#include "common/Types.hpp"


namespace emu {

class GBA;

class Timer final {
public:

    Timer(GBA &core);

    void reset();

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

private:

    u64 m_timer_start[4];
    u16 m_timer_counter[4];
    u16 m_timer_reload[4];
    u16 m_tmcnt[4];

    GBA &m_core;

    auto isTimerRunning(u8 timer) -> bool;
    auto getTimerIntermediateValue(u8 timer, bool running) -> u16;
    void updateTimer(u8 timer, u8 old_tmcnt);
    void startTimer(u8 timer);
    void stopTimer(u8 timer);
    void timerOverflowEvent(u8 timer, u32 current, u32 cycles_late);
    void timerOverflow(u8 timer);
};

} //namespace emu