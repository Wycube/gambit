#include "PPU.hpp"
#include "common/Log.hpp"


namespace emu {

PPU::PPU(Scheduler &scheduler) : m_scheduler(scheduler) {
    m_scheduler.addEvent([&] (u32 a, u32 b) { run(a, b); }, 5);
}

void PPU::run(u32 current, u32 late) {
    LOG_DEBUG("Yeet | Cycles {}, Late: {}", current, late);

    m_scheduler.addEvent([&] (u32 current, u32 late) { run(current, late); }, 5);
}

} //namespace emu