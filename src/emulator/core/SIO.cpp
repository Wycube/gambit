#include "SIO.hpp"
#include "GBA.hpp"
#include "common/Log.hpp"


namespace emu {

SIO::SIO(GBA &core) : core(core) {
    reset();
}

void SIO::reset() {
    siocnt = 0;
    rcnt = 0;
    event = core.scheduler.generateHandle();
    LOG_DEBUG("SIO has event handle: {}", event);
}

auto SIO::read8(u32 address) -> u8 {
    switch(address) {
        case 0x128: return bits::get<0, 8>(siocnt);
        case 0x129: return bits::get<8, 8>(siocnt);
        case 0x134: return bits::get<0, 8>(rcnt);
        case 0x135: return bits::get<8, 8>(rcnt);
    }

    return 0;
}

void SIO::write8(u32 address, u8 value) {
    switch(address) {
        case 0x128: siocnt = (siocnt & 0xFF00) | value; 
            if(value & 1 && value & 0x80) {
                scheduleDummyTransfer();
            }
            break;
        case 0x129: siocnt = (siocnt & 0x00FF) | ((value & 0x7F) << 8); break;
        case 0x134: rcnt = (rcnt & 0xFF00) | value; break;
        case 0x135: rcnt = (rcnt & 0x00FF) | ((value & 0x3E) << 8); break;
    }
}

void SIO::scheduleDummyTransfer() {
    //256KiHz or 2MiHz each bit for a single byte or word
    int transfer_length = bits::get_bit<12>(siocnt) ? 32 : 8;
    u64 cycles = (bits::get_bit<0>(siocnt) ? 64 : 8) * transfer_length;
    
    core.scheduler.addEvent(event, cycles, [this](u64) {
        //Disable start bit
        siocnt &= ~0x80;

        if(bits::get_bit<14>(siocnt)) {
            core.cpu.requestInterrupt(INT_SERIAL);
        }
    });
}

} //namespace emu