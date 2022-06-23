#include "DMA.hpp"
#include "common/Bits.hpp"
#include "common/Log.hpp"


namespace emu {

DMA::DMA(Scheduler &scheduler, Bus &bus) : m_scheduler(scheduler), m_bus(bus) {
    reset();
}

void DMA::reset() {
    memset(m_dma_active, false, sizeof(m_dma_active));
    memset(m_dmadad, 0, sizeof(m_dmadad));
    memset(m_dmasad, 0, sizeof(m_dmasad));
    memset(m_dmacnt_l, 0, sizeof(m_dmacnt_l));
    memset(m_dmacnt_h, 0, sizeof(m_dmacnt_h));
}

auto DMA::running() -> bool {
    return m_dma_active[0] || m_dma_active[1] || m_dma_active[2] || m_dma_active[3];
}

auto DMA::read8(u32 address) -> u8 {
    switch(address) {
        case 0xBA : return bits::get<0, 8>(m_dmacnt_h[0]);
        case 0xBB : return bits::get<8, 8>(m_dmacnt_h[0]);
        case 0xC6 : return bits::get<0, 8>(m_dmacnt_h[1]);
        case 0xC7 : return bits::get<8, 8>(m_dmacnt_h[1]);
        case 0xD2 : return bits::get<0, 8>(m_dmacnt_h[2]);
        case 0xD3 : return bits::get<8, 8>(m_dmacnt_h[2]);
        case 0xDE : return bits::get<0, 8>(m_dmacnt_h[3]);
        case 0xDF : return bits::get<8, 8>(m_dmacnt_h[3]);
    }

    return 0;
}

void DMA::write8(u32 address, u8 value) {
    u8 dma_n = (address - 0xB0) / 12;
    u8 index = address % 12;

    switch(index) {
        case 0x0 :  bits::set<0, 8>(m_dmasad[dma_n], value); break;
        case 0x1 :  bits::set<8, 8>(m_dmasad[dma_n], value); break;
        case 0x2 :  bits::set<16, 8>(m_dmasad[dma_n], value); break;
        case 0x3 :  bits::set<24, 8>(m_dmasad[dma_n], value); break;
        case 0x4 :  bits::set<0, 8>(m_dmadad[dma_n], value); break;
        case 0x5 :  bits::set<8, 8>(m_dmadad[dma_n], value); break;
        case 0x6 :  bits::set<16, 8>(m_dmadad[dma_n], value); break;
        case 0x7 :  bits::set<24, 8>(m_dmadad[dma_n], value); break;
        case 0x8 :  bits::set<0, 8>(m_dmacnt_l[dma_n], value); break;
        case 0x9 :  bits::set<8, 8>(m_dmacnt_l[dma_n], value); break;
        case 0xA :  bits::set<0, 8>(m_dmacnt_h[dma_n], value); break;
        case 0xB :  bits::set<8, 8>(m_dmacnt_h[dma_n], value); break;
    }

    //Schedule any started DMAs
    for(int i = 0; i < 4; i++) {
        if(bits::get<15, 1>(m_dmacnt_h[i])) {
            LOG_DEBUG("DMA {} enabled", i);
            LOG_FATAL("DMA start timing {}", bits::get<13, 2>(m_dmacnt_h[i]));
        }
    }

    //LOG_DEBUG("Write to DMA {} address: 0x040000{:02X}", dma_n, address);
}

} //namespace emu