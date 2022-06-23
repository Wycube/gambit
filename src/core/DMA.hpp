#pragma once

#include "core/Scheduler.hpp"
#include "core/mem/Bus.hpp"
#include "common/Types.hpp"


namespace emu {

class DMA {
public:

    DMA(Scheduler &scheduler, Bus &bus);

    void reset();
    auto running() -> bool;

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

private:

    Scheduler &m_scheduler; 
    Bus &m_bus;

    bool m_dma_active[4];
    u32 m_dmasad[4];
    u32 m_dmadad[4];
    u16 m_dmacnt_l[4];
    u16 m_dmacnt_h[4];
};

} //namespace emu