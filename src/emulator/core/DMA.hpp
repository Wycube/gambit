#pragma once

#include "common/Types.hpp"


namespace emu {

class GBA;

class DMA final {
public:

    DMA(GBA &core);

    void reset();
    auto running() -> bool;

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

    void onHBlank();
    void onVBlank();

private:

    GBA &m_core;

    struct DMAChannel {
        bool active;
        u32 source, destination;
        u16 length, control;
        
        //Internal Registers
        u32 _source, _destination;
        u16 _length;
        u32 source_address_mask, destination_address_mask;
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