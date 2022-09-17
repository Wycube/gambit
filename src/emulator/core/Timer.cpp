#include "Timer.hpp"
#include "emulator/core/GBA.hpp"
#include "common/Bits.hpp"
#include "common/Log.hpp"

constexpr u32 PRESCALER_SELECTIONS[4] = {1, 64, 256, 1024};


namespace emu {

Timer::Timer(GBA &core) : m_core(core) {
    reset();
}

void Timer::reset() {
    std::memset(m_timer_start, 0, sizeof(m_timer_start));
    std::memset(m_timer_counter, 0, sizeof(m_timer_counter));
    std::memset(m_timer_reload, 0, sizeof(m_timer_reload));
    std::memset(m_tmcnt, 0, sizeof(m_tmcnt));
}

auto Timer::read8(u32 address) -> u8 {
    switch(address) {
        case 0x100 : return bits::get<0, 8>(getTimerIntermediateValue(0, isTimerRunning(0)));
        case 0x101 : return bits::get<8, 8>(getTimerIntermediateValue(0, isTimerRunning(0)));
        case 0x102 : return bits::get<0, 8>(m_tmcnt[0]);
        case 0x103 : return bits::get<8, 8>(m_tmcnt[0]);
        case 0x104 : return bits::get<0, 8>(getTimerIntermediateValue(1, isTimerRunning(1)));
        case 0x105 : return bits::get<8, 8>(getTimerIntermediateValue(1, isTimerRunning(1)));
        case 0x106 : return bits::get<0, 8>(m_tmcnt[1]);
        case 0x107 : return bits::get<8, 8>(m_tmcnt[1]);
        case 0x108 : return bits::get<0, 8>(getTimerIntermediateValue(2, isTimerRunning(2)));
        case 0x109 : return bits::get<8, 8>(getTimerIntermediateValue(2, isTimerRunning(2)));
        case 0x10A : return bits::get<0, 8>(m_tmcnt[2]);
        case 0x10B : return bits::get<8, 8>(m_tmcnt[2]);
        case 0x10C : return bits::get<0, 8>(getTimerIntermediateValue(3, isTimerRunning(3)));
        case 0x10D : return bits::get<8, 8>(getTimerIntermediateValue(3, isTimerRunning(3)));
        case 0x10E : return bits::get<0, 8>(m_tmcnt[3]);
        case 0x10F : return bits::get<8, 8>(m_tmcnt[3]);
    }

    return 0;
}

void Timer::write8(u32 address, u8 value) {
    u8 old_tmcnt;

    switch(address) {
        case 0x100 : bits::set<0, 8>(m_timer_reload[0], value); break;
        case 0x101 : bits::set<8, 8>(m_timer_reload[0], value); break;
        case 0x102 : 
            old_tmcnt = m_tmcnt[0];
            bits::set<0, 8>(m_tmcnt[0], value);
            updateTimer(0, old_tmcnt);
            break;
        case 0x103 : bits::set<8, 8>(m_tmcnt[0], value); break;
        case 0x104 : bits::set<0, 8>(m_timer_reload[1], value); break;
        case 0x105 : bits::set<8, 8>(m_timer_reload[1], value); break;
        case 0x106 : 
            old_tmcnt = m_tmcnt[1];
            bits::set<0, 8>(m_tmcnt[1], value);
            updateTimer(1, old_tmcnt);
            break;
        case 0x107 : bits::set<8, 8>(m_tmcnt[1], value); break;
        case 0x108 : bits::set<0, 8>(m_timer_reload[2], value); break;
        case 0x109 : bits::set<8, 8>(m_timer_reload[2], value); break;
        case 0x10A : 
            old_tmcnt = m_tmcnt[2];
            bits::set<0, 8>(m_tmcnt[2], value);
            updateTimer(2, old_tmcnt);
            break;
        case 0x10B : bits::set<8, 8>(m_tmcnt[2], value); break;
        case 0x10C : bits::set<0, 8>(m_timer_reload[3], value); break;
        case 0x10D : bits::set<8, 8>(m_timer_reload[3], value); break;
        case 0x10E : 
            old_tmcnt = m_tmcnt[3];
            bits::set<0, 8>(m_tmcnt[3], value);
            updateTimer(3, old_tmcnt);
            break;
        case 0x10F : bits::set<8, 8>(m_tmcnt[3], value); break;
    }
}

auto Timer::isTimerRunning(int timer) -> bool {
    return bits::get_bit<7>(m_tmcnt[timer]) && !(bits::get_bit<2>(m_tmcnt[timer]) && timer != 0);
}

auto Timer::getTimerIntermediateValue(int timer, bool running) -> u16 {
    if(running) {
        //Calculate value based on how long it's been running
        return m_timer_counter[timer] + (m_core.scheduler.getCurrentTimestamp() - m_timer_start[timer]) / PRESCALER_SELECTIONS[bits::get<0, 2>(m_tmcnt[timer])];
    } else {
        return m_timer_counter[timer];
    }
}

void Timer::updateTimer(int timer, u8 old_tmcnt) {
    bool new_enable = bits::get_bit<7>(m_tmcnt[timer]);
    bool old_enable = bits::get_bit<7>(old_tmcnt);
    bool new_cascade = bits::get_bit<2>(m_tmcnt[timer]);
    bool old_cascade = bits::get_bit<2>(old_tmcnt);

    if(new_enable && !old_enable) {
        m_timer_counter[timer] = m_timer_reload[timer];
    }

    //Started by setting the enable bit while cascade is false, or by unsetting the cascade bit while enable is true.
    //Stopped by unsetting the enable bit while cascade is false, or by setting the cascade bit while enable was already true.
    if((new_enable && !old_enable && !new_cascade) || (!new_cascade && old_cascade && new_enable)) {
        startTimer(timer);
    } else if((!new_enable && old_enable && !new_cascade) || (new_cascade && !old_cascade && new_enable && old_enable)) {
        stopTimer(timer);
    }
}

void Timer::startTimer(int timer) {
    LOG_DEBUG("Timer {} started", timer);

    //Cascade, timer is incremented when the preceding one overflows
    if(bits::get_bit<2>(m_tmcnt[timer]) && timer != 0) {
        return;
    }

    u32 cycles_till_overflow = (0x10000 - m_timer_counter[timer]) * PRESCALER_SELECTIONS[bits::get<0, 2>(m_tmcnt[timer])];
    m_timer_start[timer] = m_core.scheduler.getCurrentTimestamp();
    m_core.scheduler.addEvent(fmt::format("Timer {} Overflow", timer), [this, timer](u32 a, u32 b) { timerOverflowEvent(timer, a, b); }, cycles_till_overflow);
}

void Timer::stopTimer(int timer) {
    LOG_DEBUG("Timer {} stopped", timer);

    m_timer_counter[timer] = getTimerIntermediateValue(timer, true);
    m_core.scheduler.removeEvent(fmt::format("Timer {} Overflow", timer));
}

void Timer::timerOverflowEvent(int timer, u32 current, u32 cycles_late) {
    timerOverflow(timer);

    u32 cycles_till_overflow = (0x10000 - m_timer_counter[timer]) * PRESCALER_SELECTIONS[bits::get<0, 2>(m_tmcnt[timer])];
    m_timer_start[timer] = m_core.scheduler.getCurrentTimestamp();
    m_core.scheduler.addEvent(fmt::format("Timer {} Overflow", timer), [this, timer](u32 a, u32 b) { timerOverflowEvent(timer, a, b); }, cycles_till_overflow - cycles_late);
}

void Timer::timerOverflow(int timer) {
    m_timer_counter[timer] = m_timer_reload[timer];
 
    if(timer < 3 && bits::get_bit<2>(m_tmcnt[timer + 1])) {
        if(++m_timer_counter[timer + 1] == 0) {
            timerOverflow(timer + 1);
        }
    }

    //Request Interrupt if enabled
    if(bits::get_bit<6>(m_tmcnt[timer])) {
        m_core.bus.requestInterrupt(static_cast<InterruptSource>(INT_TIM_0 << timer));
    }
}

} //namespace emu