#pragma once

#include "Scheduler.hpp"
#include "common/Types.hpp"


namespace emu {

class GBA;

class DMA final {
public:

    explicit DMA(GBA &core);

    void reset();
    auto running() -> bool;
    void step(u32 cycles);

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

    void onHBlank();
    void onVBlank();
    void onVideoCapture();
    void disableVideoCapture();
    void onTimerOverflow(int fifo);

private:

    void startTransfer(int dma_n);
    
    GBA &core;

    struct DMAChannel {
        bool active;
        u32 source, destination;
        u16 length, control;
        
        //Internal Registers
        u32 _source, _destination;
        u32 _length;

        EventHandle event;
    } channel[4];
};

} //namespace emu