#include "Timer.hpp"
#include "emulator/core/GBA.hpp"
#include "common/Bits.hpp"
#include "common/Log.hpp"

constexpr u32 PRESCALER_SELECTIONS[4] = {1, 64, 256, 1024};


namespace emu {

Timer::Timer(GBA &core) : core(core) {
    reset();
}

void Timer::reset() {
    std::memset(timer_start, 0, sizeof(timer_start));
    std::memset(timer_counter, 0, sizeof(timer_counter));
    std::memset(timer_reload, 0, sizeof(timer_reload));
    std::memset(tmcnt, 0, sizeof(tmcnt));

    for(size_t i = 0; i < 4; i++) {
        timer_events[i] = core.scheduler.generateHandle();
        LOG_DEBUG("Timer {} has event handle: {}", i, timer_events[i]);
    }
}

auto Timer::read8(u32 address) -> u8 {
    switch(address) {
        case 0x100 : return bits::get<0, 8>(getTimerIntermediateValue(0, isTimerRunning(0)));
        case 0x101 : return bits::get<8, 8>(getTimerIntermediateValue(0, isTimerRunning(0)));
        case 0x102 : return bits::get<0, 8>(tmcnt[0]);
        case 0x103 : return bits::get<8, 8>(tmcnt[0]);
        case 0x104 : return bits::get<0, 8>(getTimerIntermediateValue(1, isTimerRunning(1)));
        case 0x105 : return bits::get<8, 8>(getTimerIntermediateValue(1, isTimerRunning(1)));
        case 0x106 : return bits::get<0, 8>(tmcnt[1]);
        case 0x107 : return bits::get<8, 8>(tmcnt[1]);
        case 0x108 : return bits::get<0, 8>(getTimerIntermediateValue(2, isTimerRunning(2)));
        case 0x109 : return bits::get<8, 8>(getTimerIntermediateValue(2, isTimerRunning(2)));
        case 0x10A : return bits::get<0, 8>(tmcnt[2]);
        case 0x10B : return bits::get<8, 8>(tmcnt[2]);
        case 0x10C : return bits::get<0, 8>(getTimerIntermediateValue(3, isTimerRunning(3)));
        case 0x10D : return bits::get<8, 8>(getTimerIntermediateValue(3, isTimerRunning(3)));
        case 0x10E : return bits::get<0, 8>(tmcnt[3]);
        case 0x10F : return bits::get<8, 8>(tmcnt[3]);
    }

    return 0;
}

void Timer::write8(u32 address, u8 value) {
    u8 old_tmcnt;

    switch(address) {
        case 0x100 : bits::set<0, 8>(timer_reload[0], value); break;
        case 0x101 : bits::set<8, 8>(timer_reload[0], value); break;
        case 0x102 :
            old_tmcnt = tmcnt[0];
            //On Timer 0 bit 2 is always 0
            bits::set<0, 8>(tmcnt[0], value & ~4);
            updateTimer(0, old_tmcnt);
            break;
        case 0x103 : bits::set<8, 8>(tmcnt[0], value); break;
        case 0x104 : bits::set<0, 8>(timer_reload[1], value); break;
        case 0x105 : bits::set<8, 8>(timer_reload[1], value); break;
        case 0x106 :
            old_tmcnt = tmcnt[1];
            bits::set<0, 8>(tmcnt[1], value);
            updateTimer(1, old_tmcnt);
            break;
        case 0x107 : bits::set<8, 8>(tmcnt[1], value); break;
        case 0x108 : bits::set<0, 8>(timer_reload[2], value); break;
        case 0x109 : bits::set<8, 8>(timer_reload[2], value); core.scheduler.step(1); break;
        case 0x10A :
            old_tmcnt = tmcnt[2];
            bits::set<0, 8>(tmcnt[2], value);
            updateTimer(2, old_tmcnt);
            break;
        case 0x10B : bits::set<8, 8>(tmcnt[2], value); break;
        case 0x10C : bits::set<0, 8>(timer_reload[3], value); break;
        case 0x10D : bits::set<8, 8>(timer_reload[3], value); core.scheduler.step(1); break;
        case 0x10E :
            old_tmcnt = tmcnt[3];
            bits::set<0, 8>(tmcnt[3], value);
            updateTimer(3, old_tmcnt);
            break;
        case 0x10F : bits::set<8, 8>(tmcnt[3], value); break;
    }
}

auto Timer::isTimerRunning(u8 timer) -> bool {
    return bits::get_bit<7>(tmcnt[timer]) && !(bits::get_bit<2>(tmcnt[timer]) && timer != 0);
}

auto Timer::getTimerIntermediateValue(u8 timer, bool running) -> u16 {
    if(running) {
        //Calculate value based on how long it's been running
        return timer_counter[timer] + (core.scheduler.getCurrentTimestamp() - timer_start[timer]) / PRESCALER_SELECTIONS[bits::get<0, 2>(tmcnt[timer])];
    } else {
        return timer_counter[timer];
    }
}

void Timer::updateTimer(u8 timer, u8 old_tmcnt) {
    bool new_enable = bits::get_bit<7>(tmcnt[timer]);
    bool old_enable = bits::get_bit<7>(old_tmcnt);
    bool new_cascade = bits::get_bit<2>(tmcnt[timer]);
    bool old_cascade = bits::get_bit<2>(old_tmcnt);

    if(new_enable && !old_enable) {
        timer_counter[timer] = timer_reload[timer];
    }

    //Started by setting the enable bit while cascade is false, or by unsetting the cascade bit while enable is true.
    //Stopped by unsetting the enable bit while cascade is false, or by setting the cascade bit while enable was already true.
    if((new_enable && !old_enable && !new_cascade) || (!new_cascade && old_cascade && new_enable)) {
        startTimer(timer);
    } else if((!new_enable && old_enable && !new_cascade) || (new_cascade && !old_cascade && new_enable && old_enable)) {
        stopTimer(timer);
    }
}

void Timer::startTimer(u8 timer) {
    LOG_TRACE("Timer {} started", timer);

    //Cascade, timer is incremented when the preceding one overflows (so it doesn't actually run)
    if(bits::get_bit<2>(tmcnt[timer]) && timer != 0) {
        return;
    }

    //2-cycle delay before timer starts
    core.scheduler.addEvent(timer_events[timer], 2, [this, timer](u64 late) {
        u64 cycles_till_overflow = (0x10000 - timer_counter[timer]) * PRESCALER_SELECTIONS[bits::get<0, 2>(tmcnt[timer])];
        timer_start[timer] = core.scheduler.getCurrentTimestamp();
        core.scheduler.addEvent(timer_events[timer], cycles_till_overflow, [this, timer](u64 late) {
            timerOverflowEvent(timer, late);
        });
    });
}

void Timer::stopTimer(u8 timer) {
    LOG_TRACE("Timer {} stopped", timer);

    core.scheduler.step(2);
    timer_counter[timer] = getTimerIntermediateValue(timer, true);
    core.scheduler.removeEvent(timer_events[timer]);
}

void Timer::timerOverflowEvent(u8 timer, u64 late) {
    timerOverflow(timer);

    u64 cycles_till_overflow = (0x10000 - timer_counter[timer]) * PRESCALER_SELECTIONS[bits::get<0, 2>(tmcnt[timer])];
    timer_start[timer] = core.scheduler.getCurrentTimestamp();
    core.scheduler.addEvent(timer_events[timer], cycles_till_overflow - late, [this, timer](u64 late) {
        timerOverflowEvent(timer, late);
    });
}

void Timer::timerOverflow(u8 timer) {
    timer_counter[timer] = timer_reload[timer];

    if(core.apu.isTimerSelected(timer)) {
        core.apu.onTimerOverflow(timer);
    }
 
    if(timer < 3 && bits::get_bit<2>(tmcnt[timer + 1])) {
        if(++timer_counter[timer + 1] == 0) {
            timerOverflow(timer + 1);
        }
    }

    //Request Interrupt if enabled
    if(bits::get_bit<6>(tmcnt[timer])) {
        core.bus.requestInterrupt(static_cast<InterruptSource>(INT_TIM_0 << timer));
    }
}

} //namespace emu