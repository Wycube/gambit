#pragma once

#include "emulator/core/Scheduler.hpp"
#include "emulator/core/mem/Bus.hpp"
#include "common/Types.hpp"


namespace emu {

class DMA final {
public:

    DMA(Scheduler &scheduler, Bus &bus);

    void reset();
    auto running() -> bool;

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

    void onHBlank();
    void onVBlank();

private:

    Scheduler &m_scheduler; 
    Bus &m_bus;

    struct DMAChannel {
        bool active;
        u32 source, destination;
        u16 length, control;
        
        //Internal Registers
        u32 _source, _destination;
        u16 _length;
    } m_channel[4];

    // bool m_dma_active[4];
    // u32 m_dmasad[4];
    // u32 m_dmadad[4];
    // u16 m_dmalength[4];
    // u16 m_dmacnt_h[4];

    void startTransfer(int dma_n);
    template<typename T>
    void transfer(int dma_n, u32 current, u32 cycles_late);
};

} //namespace emu