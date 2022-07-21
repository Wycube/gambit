#pragma once

#include "emulator/core/Scheduler.hpp"
#include "common/Types.hpp"


namespace emu {

class Bus;


class Timer {
public:

    Timer(Scheduler &scheduler, Bus &bus);

    void reset();

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

private:

    u32 m_timer_start[4];
    u16 m_timer_counter[4];
    u16 m_timer_reload[4];
    u16 m_tmcnt[4];

    Scheduler &m_scheduler;
    Bus &m_bus;

    auto isTimerRunning(int timer) -> bool;
    auto getTimerIntermediateValue(int timer, bool running) -> u16;
    void updateTimer(int timer, u8 old_tmcnt);
    void startTimer(int timer);
    void stopTimer(int timer);
    void timerOverflowEvent(int timer, u32 current, u32 cycles_late);
    void timerOverflow(int timer);
};

} //namespace emu