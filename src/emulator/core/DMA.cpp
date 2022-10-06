#include "DMA.hpp"
#include "emulator/core/GBA.hpp"
#include "common/Bits.hpp"
#include "common/Log.hpp"


//Internal Memory (27 bit address) or Any Memory (28 bit address)
static constexpr u32 source_address_mask[4] = {0x07FFFFFF, 0x0FFFFFFF, 0x0FFFFFFF, 0x0FFFFFFF};
static constexpr u32 destination_address_mask[4] = {0x07FFFFFF, 0x07FFFFFF, 0x07FFFFFF, 0x0FFFFFFF};
static constexpr u16 length_mask[4] = {0x3FFF, 0x3FFF, 0x3FFF, 0xFFFF};

namespace emu {

DMA::DMA(GBA &core) : m_core(core) {
    reset();
}

void DMA::reset() {
    for(int i = 0; i < 4; i++) {
        m_channel[i].active = false;
        m_channel[i].source = 0;
        m_channel[i].destination = 0;
        m_channel[i].length = 0;
        m_channel[i].control = 0;
    }
}

auto DMA::running() -> bool {
    return m_channel[0].active || m_channel[1].active || m_channel[2].active || m_channel[3].active;
}

auto DMA::read8(u32 address) -> u8 {
    u8 dma_n = (address - 0xB0) / 12;
    u8 index = (address - 0xB0) % 12;

    switch(index) {
        case 0xA : return bits::get<0, 8>(m_channel[dma_n].control);
        case 0xB : return bits::get<8, 8>(m_channel[dma_n].control);
    }

    return 0;
}

void DMA::write8(u32 address, u8 value) {
    u8 dma_n = (address - 0xB0) / 12;
    u8 index = (address - 0xB0) % 12;
    bool old_enable = bits::get_bit<15>(m_channel[dma_n].control);

    switch(index) {
        case 0x0 :  bits::set<0, 8>(m_channel[dma_n].source, value); break;
        case 0x1 :  bits::set<8, 8>(m_channel[dma_n].source, value); break;
        case 0x2 :  bits::set<16, 8>(m_channel[dma_n].source, value); break;
        case 0x3 :  bits::set<24, 8>(m_channel[dma_n].source, value); break;
        case 0x4 :  bits::set<0, 8>(m_channel[dma_n].destination, value); break;
        case 0x5 :  bits::set<8, 8>(m_channel[dma_n].destination, value); break;
        case 0x6 :  bits::set<16, 8>(m_channel[dma_n].destination, value); break;
        case 0x7 :  bits::set<24, 8>(m_channel[dma_n].destination, value); break;
        case 0x8 :  bits::set<0, 8>(m_channel[dma_n].length, value); break;
        case 0x9 :  bits::set<8, 8>(m_channel[dma_n].length, value); break;
        case 0xA :  bits::set<0, 8>(m_channel[dma_n].control, value & 0xE0); break;
        case 0xB :  bits::set<8, 8>(m_channel[dma_n].control, value & (dma_n == 3 ? 0xFF : 0xF7)); break;
    }

    //Schedule any started DMAs
    u32 control = m_channel[dma_n].control;

    if(bits::get_bit<15>(control) && !old_enable) {
        if(bits::get<12, 2>(control) == 3) {
            // LOG_WARNING("DMA {} start timing {} not supported yet", dma_n, bits::get<12, 2>(control));
            return;
        }

        LOG_DEBUG("DMA {} enabled", dma_n);
        LOG_DEBUG("DMA start timing {}", bits::get<12, 2>(control));
        LOG_DEBUG("DMA destination control {}", bits::get<5, 2>(control));
        LOG_DEBUG("DMA source control {}", bits::get<7, 2>(control));
        LOG_DEBUG("DMA source address      : {:08X}", m_channel[dma_n].source);
        LOG_DEBUG("DMA destination address : {:08X}", m_channel[dma_n].destination);
        LOG_DEBUG("DMA transfer length     : {}", m_channel[dma_n].length == 0 ? dma_n == 3 ? 0x10000 : 0x4000 : m_channel[dma_n].length);
        LOG_DEBUG("DMA transfer size       : {}", bits::get_bit<10>(control) ? 32 : 16);
        LOG_DEBUG("DMA repeat              : {}", bits::get_bit<9>(control));
        LOG_DEBUG("DMA irq on finish       : {}", bits::get_bit<14>(control));
    
        //Reload internal registers
        m_channel[dma_n]._source = m_channel[dma_n].source & ~(bits::get_bit<10>(m_channel[dma_n].control) ? 3 : 1);
        m_channel[dma_n]._destination = m_channel[dma_n].destination & ~(bits::get_bit<10>(m_channel[dma_n].control) ? 3 : 1);
        m_channel[dma_n]._length = m_channel[dma_n].length;

        if(bits::get<12, 2>(m_channel[dma_n].control) == 0) {
            startTransfer(dma_n);
        }
    }

    //LOG_DEBUG("Write to DMA {} address: 0x040000{:02X}", dma_n, address);
}

void DMA::onHBlank() {
    for(int i = 0; i < 4; i++) {
        if(!m_channel[i].active && bits::get_bit<15>(m_channel[i].control) && bits::get<12, 2>(m_channel[i].control) == 2) {
            startTransfer(i);
        }
    }
}

void DMA::onVBlank() {
    for(int i = 0; i < 4; i++) {
        if(!m_channel[i].active && bits::get_bit<15>(m_channel[i].control) && bits::get<12, 2>(m_channel[i].control) == 1) {
            startTransfer(i);
        }
    }
}

void DMA::startTransfer(int dma_n) {
    //Assume start timing 0 (immediately) for now
    u32 length = m_channel[dma_n].length == 0 ? dma_n == 3 ? 0x10000 : 0x4000 : m_channel[dma_n].length;
    u32 transfer_time = 6 + (length - 1) * 2; //2N + (n - 1)S + xI + 2 cycles before the transfer starts
    bool transfer_size = bits::get_bit<10>(m_channel[dma_n].control);
    m_channel[dma_n].active = true;

    m_core.scheduler.addEvent("DMA Start", [this, dma_n](u32, u32) { m_channel[dma_n].active = true; }, 2);

    m_core.scheduler.addEvent("DMA Transfer", [this, dma_n, transfer_size](u32 a, u32 b) {
        if(transfer_size) {
            transfer<u32>(dma_n, a, b);
        } else {
            transfer<u16>(dma_n, a, b);
        }
    }, transfer_time);
}

void adjustAddress(u32 &address, u8 adjust_type, u8 amount) {
    assert(adjust_type < 4);
    
    switch(adjust_type) {
        case 0 : address += amount; break; //Increment
        case 1 : address -= amount; break; //Decrement
        //2 is fixed, so nothing changes
        case 3 : address += amount; break; //Increment/Reload
    }
}

template<typename T>
void DMA::transfer(int dma_n, u32 current, u32 cycles_late) {
    static_assert(sizeof(T) == 2 || sizeof(T) == 4);
    u32 source = m_channel[dma_n]._source & source_address_mask[dma_n];
    u32 destination = m_channel[dma_n]._destination & destination_address_mask[dma_n];
    u32 control = source >= 0x08000000 ? m_channel[dma_n].control & ~0x180 : m_channel[dma_n].control;
    int length = m_channel[dma_n]._length == 0 ? dma_n == 3 ? 0x10000 : 0x4000 : m_channel[dma_n]._length & length_mask[dma_n];
    LOG_DEBUG("WOAH, DMA-ing from {:08X} to {:08X} with word count {} and transfer size {} bytes", source, destination, length, sizeof(T));

    for(int i = 0; i < length; i++) {
        if constexpr(sizeof(T) == 2) {
            m_core.bus.debugWrite16(destination, m_core.bus.debugRead16(source));
            adjustAddress(source, bits::get<7, 2>(control), 2);
            adjustAddress(destination, bits::get<5, 2>(control), 2);
        } else if constexpr(sizeof(T) == 4) {
            m_core.bus.debugWrite32(destination, m_core.bus.debugRead32(source));
            adjustAddress(source, bits::get<7, 2>(control), 4);
            adjustAddress(destination, bits::get<5, 2>(control), 4);
        }
    }

    m_channel[dma_n]._source = source;
    m_channel[dma_n]._destination = destination;

    m_channel[dma_n].active = false;

    //TODO: Repeat Bit, more than this at least
    if(bits::get_bit<9>(control)) {
        if(bits::get<5, 2>(control) == 3) {
            m_channel[dma_n]._destination = m_channel[dma_n].destination & ~(sizeof(T) == 4 ? 3 : 1);
        }
        m_channel[dma_n]._length = m_channel[dma_n].length;
    } else {
        m_channel[dma_n].control &= ~0x8000;
    }


    if(bits::get_bit<14>(control)) {
        m_core.bus.requestInterrupt(static_cast<InterruptSource>(INT_DMA_0 << dma_n));
    }
}

} //namespace emu