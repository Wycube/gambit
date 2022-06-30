#pragma once

#include "emulator/core/Scheduler.hpp"
#include "common/Types.hpp"


namespace emu {

class Timer {
public:

    Timer(Scheduler &scheduler);

    void reset();

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

private:

    u16 m_tmcnt_l[4];
    u16 m_tmcnt_h[4];

    Scheduler &m_scheduler;
};

} //namespace emu