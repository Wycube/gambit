#include "DMA.hpp"
#include "emulator/core/GBA.hpp"
#include "common/Bits.hpp"
#include "common/Log.hpp"

//Internal Memory (27-bit address) or Any Memory except SRAM (28-bit address)
static constexpr u32 SOURCE_ADDRESS_MASK[4] = {0x07FFFFFF, 0x0FFFFFFF, 0x0FFFFFFF, 0x0FFFFFFF};
static constexpr u32 DESTINATION_ADDRESS_MASK[4] = {0x07FFFFFF, 0x07FFFFFF, 0x07FFFFFF, 0x0FFFFFFF};
static constexpr u16 LENGTH_MASK[4] = {0x3FFF, 0x3FFF, 0x3FFF, 0xFFFF};


namespace emu {

DMA::DMA(GBA &core) : core(core) {
    for(size_t i = 0; i < 4; i++) {
        channel[i].event = core.scheduler.registerEvent([this, i](u64) {
            channel[i].active = true;
            LOG_TRACE("DMA {} started on cycle: {}", i, this->core.scheduler.getCurrentTimestamp());
        });
        LOG_DEBUG("DMA {} has event handle: {}", i, channel[i].event);
    }

    reset();
}

void DMA::reset() {
    for(size_t i = 0; i < 4; i++) {
        channel[i].active = false;
        channel[i].source = 0;
        channel[i].destination = 0;
        channel[i].length = 0;
        channel[i].control = 0;
    }
}

auto DMA::running() -> bool {
    return channel[0].active || channel[1].active || channel[2].active || channel[3].active;
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

void DMA::step(u32 cycles) {
    u64 target = core.scheduler.getCurrentTimestamp() + cycles;

    while(core.scheduler.getCurrentTimestamp() < target) {
        //Get current DMA channel highest priority
        int current = -1;
        for(int i = 0; i < 4; i++) {
            if(channel[i].active) {
                current = i;
                break;
            }
        }

        if(current == -1) {
            break;
        }

        bool audio_dma = (current == 1 || current == 2) && bits::get<12, 2>(channel[current].control) == 3;
        bool transfer_size = bits::get_bit<10>(channel[current].control) | audio_dma;
        u32 control = channel[current]._source >= 0x08000000 ? channel[current].control & ~0x180 : channel[current].control;

        //TODO: Proper timings N/S cycles
        if(transfer_size) {
            core.bus.write32(channel[current]._destination, core.bus.read32(channel[current]._source, SEQUENTIAL), SEQUENTIAL);
            adjustAddress(channel[current]._source, bits::get<7, 2>(control), 4);
            adjustAddress(channel[current]._destination, audio_dma ? 2 : bits::get<5, 2>(control), 4);
        } else {
            core.bus.write16(channel[current]._destination, core.bus.read16(channel[current]._source, SEQUENTIAL), SEQUENTIAL);
            adjustAddress(channel[current]._source, bits::get<7, 2>(control), 2);
            adjustAddress(channel[current]._destination, bits::get<5, 2>(control), 2);
        }

        channel[current]._length--;

        if(channel[current]._length == 0) {
            LOG_TRACE("DMA {} finished on cycle: {}", current, core.scheduler.getCurrentTimestamp());

            channel[current].active = false;

            if(bits::get_bit<9>(control)) {
                if(bits::get<5, 2>(control) == 3) {
                    channel[current]._destination = channel[current].destination & ~(transfer_size ? 3 : 1);
                }
                channel[current]._length = channel[current].length;
            } else {
                channel[current].control &= ~0x8000;
            }

            if(bits::get_bit<14>(control)) {
                core.cpu.requestInterrupt(static_cast<InterruptSource>(INT_DMA_0 << current));
            }
        }
    }
}

auto DMA::read8(u32 address) -> u8 {
    u8 dma_n = (address - 0xB0) / 12;
    u8 index = (address - 0xB0) % 12;

    switch(index) {
        case 0xA : return bits::get<0, 8>(channel[dma_n].control);
        case 0xB : return bits::get<8, 8>(channel[dma_n].control);
    }

    return 0;
}

void DMA::write8(u32 address, u8 value) {
    u8 dma_n = (address - 0xB0) / 12;
    u8 index = (address - 0xB0) % 12;
    bool old_enable = bits::get_bit<15>(channel[dma_n].control);

    switch(index) {
        case 0x0 : bits::set<0, 8>(channel[dma_n].source, value); break;
        case 0x1 : bits::set<8, 8>(channel[dma_n].source, value); break;
        case 0x2 : bits::set<16, 8>(channel[dma_n].source, value); break;
        case 0x3 : bits::set<24, 8>(channel[dma_n].source, value); channel[dma_n].source &= SOURCE_ADDRESS_MASK[dma_n]; break;
        case 0x4 : bits::set<0, 8>(channel[dma_n].destination, value); break;
        case 0x5 : bits::set<8, 8>(channel[dma_n].destination, value); break;
        case 0x6 : bits::set<16, 8>(channel[dma_n].destination, value); break;
        case 0x7 : bits::set<24, 8>(channel[dma_n].destination, value); channel[dma_n].destination &= DESTINATION_ADDRESS_MASK[dma_n]; break;
        case 0x8 : bits::set<0, 8>(channel[dma_n].length, value); break;
        case 0x9 : bits::set<8, 8>(channel[dma_n].length, value); break;
        case 0xA : bits::set<0, 8>(channel[dma_n].control, value & 0xE0); break;
        case 0xB : bits::set<8, 8>(channel[dma_n].control, value & (dma_n == 3 ? 0xFF : 0xF7)); break;
    }

    //Schedule any started DMAs
    u32 control = channel[dma_n].control;

    if(bits::get_bit<15>(control) && !old_enable) {
        LOG_TRACE("DMA {} enabled", dma_n);
        LOG_TRACE("DMA start timing {}", bits::get<12, 2>(control));
        LOG_TRACE("DMA destination control {}", bits::get<5, 2>(control));
        LOG_TRACE("DMA source control {}", bits::get<7, 2>(control));
        LOG_TRACE("DMA source address      : {:08X}", channel[dma_n].source);
        LOG_TRACE("DMA destination address : {:08X}", channel[dma_n].destination);
        LOG_TRACE("DMA transfer length     : {}", channel[dma_n].length == 0 ? dma_n == 3 ? 0x10000 : 0x4000 : channel[dma_n].length);
        LOG_TRACE("DMA transfer size       : {}", bits::get_bit<10>(control) ? 32 : 16);
        LOG_TRACE("DMA repeat              : {}", bits::get_bit<9>(control));
        LOG_TRACE("DMA irq on finish       : {}", bits::get_bit<14>(control));
    
        //Reload internal registers
        channel[dma_n]._source = channel[dma_n].source & ~(bits::get_bit<10>(channel[dma_n].control) ? 3 : 1);
        channel[dma_n]._destination = channel[dma_n].destination & ~(bits::get_bit<10>(channel[dma_n].control) ? 3 : 1);
        channel[dma_n]._length = channel[dma_n].length == 0 ? dma_n == 3 ? 0x10000 : 0x4000 : channel[dma_n].length & LENGTH_MASK[dma_n];

        if(bits::get<12, 2>(channel[dma_n].control) == 0) {
            startTransfer(dma_n);
        }
    }
}

void DMA::onHBlank() {
    for(size_t i = 0; i < 4; i++) {
        if(!channel[i].active && bits::get_bit<15>(channel[i].control) && bits::get<12, 2>(channel[i].control) == 2) {
            startTransfer(i);
        }
    }
}

void DMA::onVBlank() {
    for(size_t i = 0; i < 4; i++) {
        if(!channel[i].active && bits::get_bit<15>(channel[i].control) && bits::get<12, 2>(channel[i].control) == 1) {
            startTransfer(i);
        }
    }
}

void DMA::onVideoCapture() {
    if(!channel[3].active && bits::get_bit<15>(channel[3].control) && bits::get<12, 2>(channel[3].control) == 3) {
        startTransfer(3);
    }
}

void DMA::disableVideoCapture() {
    if(bits::get_bit<15>(channel[3].control) && bits::get<12, 2>(channel[3].control) == 3) {
        channel[3].control &= ~0x8000;
    }
}

void DMA::onTimerOverflow(int fifo) {
    static constexpr u32 FIFO_ADDRESSES[2] = {0x040000A0, 0x040000A4};

    if(bits::get<12, 2>(channel[1].control) == 3
        && channel[1].destination == FIFO_ADDRESSES[fifo]
        && bits::get_bit<9>(channel[1].control)) {

        channel[1].control |= 0x8000;
        channel[1]._length = 4;
        startTransfer(1);
    }

    if(bits::get<12, 2>(channel[2].control) == 3
        && channel[2].destination == FIFO_ADDRESSES[fifo]
        && bits::get_bit<9>(channel[2].control)) {
        
        channel[2].control |= 0x8000;
        channel[2]._length = 4;
        startTransfer(2);
    }
}

void DMA::startTransfer(int dma_n) {
    core.scheduler.addEvent(channel[dma_n].event, 2);
}

} //namespace emu