#pragma once

#include "Scheduler.hpp"
#include "common/Types.hpp"


namespace emu {

class GBA;

class SIO final {
public:

    explicit SIO(GBA &core);

    void reset();
    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

private:

    void scheduleDummyTransfer();
    
    GBA &core;
    EventHandle event;

    u16 siocnt;
    u16 rcnt;
};

} //namespace emu