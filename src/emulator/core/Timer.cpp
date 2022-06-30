#include "Timer.hpp"
#include "common/Bits.hpp"
#include "common/Log.hpp"


namespace emu {

Timer::Timer(Scheduler &scheduler) : m_scheduler(scheduler) {
    reset();
}

void Timer::reset() {
    std::memset(m_tmcnt_l, 0, sizeof(m_tmcnt_l));
    std::memset(m_tmcnt_h, 0, sizeof(m_tmcnt_h));
}

auto Timer::read8(u32 address) -> u8 {
    switch(address) {
        case 0x100 : return bits::get<0, 8>(m_tmcnt_l[0]);
        case 0x101 : return bits::get<8, 8>(m_tmcnt_l[0]);
        case 0x102 : return bits::get<0, 8>(m_tmcnt_h[0]);
        case 0x103 : return bits::get<8, 8>(m_tmcnt_h[0]);
        case 0x104 : return bits::get<0, 8>(m_tmcnt_l[1]);
        case 0x105 : return bits::get<8, 8>(m_tmcnt_l[1]);
        case 0x106 : return bits::get<0, 8>(m_tmcnt_h[1]);
        case 0x107 : return bits::get<8, 8>(m_tmcnt_h[1]);
        case 0x108 : return bits::get<0, 8>(m_tmcnt_l[2]);
        case 0x109 : return bits::get<8, 8>(m_tmcnt_l[2]);
        case 0x10A : return bits::get<0, 8>(m_tmcnt_h[2]);
        case 0x10B : return bits::get<8, 8>(m_tmcnt_h[2]);
        case 0x10C : return bits::get<0, 8>(m_tmcnt_l[3]);
        case 0x10D : return bits::get<8, 8>(m_tmcnt_l[3]);
        case 0x10E : return bits::get<0, 8>(m_tmcnt_h[3]);
        case 0x10F : return bits::get<8, 8>(m_tmcnt_h[3]);
    }
}

void Timer::write8(u32 address, u8 value) {
    switch(address) {
        case 0x100 : bits::set<0, 8>(m_tmcnt_l[0], value);
        case 0x101 : bits::set<8, 8>(m_tmcnt_l[0], value);
        case 0x102 : bits::set<0, 8>(m_tmcnt_h[0], value);
        case 0x103 : bits::set<8, 8>(m_tmcnt_h[0], value);
        case 0x104 : bits::set<0, 8>(m_tmcnt_l[1], value);
        case 0x105 : bits::set<8, 8>(m_tmcnt_l[1], value);
        case 0x106 : bits::set<0, 8>(m_tmcnt_h[1], value);
        case 0x107 : bits::set<8, 8>(m_tmcnt_h[1], value);
        case 0x108 : bits::set<0, 8>(m_tmcnt_l[2], value);
        case 0x109 : bits::set<8, 8>(m_tmcnt_l[2], value);
        case 0x10A : bits::set<0, 8>(m_tmcnt_h[2], value);
        case 0x10B : bits::set<8, 8>(m_tmcnt_h[2], value);
        case 0x10C : bits::set<0, 8>(m_tmcnt_l[3], value);
        case 0x10D : bits::set<8, 8>(m_tmcnt_l[3], value);
        case 0x10E : bits::set<0, 8>(m_tmcnt_h[3], value);
        case 0x10F : bits::set<8, 8>(m_tmcnt_h[3], value);
    }

    LOG_DEBUG("Write to Timers");
}

} //namespace emu