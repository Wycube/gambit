#pragma once

#include "core/Scheduler.hpp"


namespace emu {

class PPU {
private:

    Scheduler &m_scheduler;

public:

    PPU(Scheduler &scheduler);

    void run(u32 current, u32 late);
};

} //namespace emu