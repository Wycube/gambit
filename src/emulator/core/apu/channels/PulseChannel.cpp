#include "PulseChannel.hpp"
#include "common/Bits.hpp"
#include "common/Log.hpp"

//12.5% |-_______-_______|
//25.0% |--______--______|
//50.0% |----____----____|
//75.0% |------__------__|
static constexpr s8 WAVE_DUTY[4][8] = {
    {1, -1, -1, -1, -1, -1, -1, -1},
    {1,  1, -1, -1, -1, -1, -1, -1},
    {1,  1,  1,  1, -1, -1, -1, -1},
    {1,  1,  1,  1,  1,  1, -1, -1}
};


//TODO: Implement rest of missing features, DAC, stuff from gb emu

namespace emu {

PulseChannel::PulseChannel(Scheduler &scheduler) : scheduler(scheduler) {
    frequency_event = scheduler.generateHandle();
    LOG_DEBUG("Pulse channel has event handle: {}", frequency_event);
}

void PulseChannel::reset() {
    sndcnt_h = 0;
    sndcnt_l = 0;
    sndcnt_x = 0;
    enabled = false;
}

auto PulseChannel::read(u32 address) -> u8 {
    switch(address) {
        case 0x60 : return sndcnt_l;

        case 0x62 :
        case 0x68 : return sndcnt_h & 0xC0;
        case 0x63 :
        case 0x69 : return sndcnt_h >> 8;

        case 0x65 :
        case 0x6D : return (sndcnt_x >> 8) & 0x40;
    }

    return 0;
}

void PulseChannel::write(u32 address, u8 value) {
        switch(address) {
        case 0x60 : sndcnt_l = value & 0x7F; break;

        case 0x62 :
        case 0x68 : sndcnt_h = (sndcnt_h & 0xFF00) | value; break;
        case 0x63 :
        case 0x69 : sndcnt_h = (sndcnt_h & 0x00FF) | (value << 8); break;

        case 0x64 :
        case 0x6C : sndcnt_x = (sndcnt_x & 0xFF00) | value; break;
        case 0x65 :
        case 0x6D : sndcnt_x = (sndcnt_x & 0x00FF) | (value << 8); 
            if(value >> 7) {
                restart();
            }
            break;
    }
}

auto PulseChannel::amplitude() -> s8 {
    if(enabled && bits::get<11, 5>(sndcnt_h) != 0) {
        return WAVE_DUTY[bits::get<6, 2>(sndcnt_h)][wave_duty_pos] * current_vol;
    }

    return 0;
}

void PulseChannel::step() {
    if(bits::get_bit<14>(sndcnt_x) && length_timer > 0 && --length_timer == 0) {
        enabled = false;
    }

    if(envelope_timer > 0 && --envelope_timer == 0) {
        const u8 envelope_period = bits::get<8, 3>(sndcnt_h) == 0 ? 8 : bits::get<8, 3>(sndcnt_h);
        envelope_timer = envelope_period * 8;

        if(bits::get_bit<11>(sndcnt_h)) {
            //Increase
            if(current_vol < 15) {
                current_vol++;
            }
        } else {
            //Decrease
            if(current_vol > 0) {
                current_vol--;
            }
        }
    }

    if(sweep_timer > 0 && --sweep_timer == 0) {
        sweep_timer = bits::get<4, 3>(sndcnt_l) * 4;

        if((sndcnt_l & 7) != 0 && bits::get<4, 3>(sndcnt_l) != 0) {
            u16 new_freq;

            if((sndcnt_l >> 3) & 1) {
                //Decreasing
                new_freq = shadow_freq - (shadow_freq >> (sndcnt_l & 7));
            } else {
                //Increasing
                new_freq = shadow_freq + (shadow_freq >> (sndcnt_l & 7));
            }

            if(new_freq > 2047) {
                enabled = false;
            } else {
                shadow_freq = new_freq;
                sndcnt_x = (sndcnt_x & ~0x7FF) | new_freq;

                //Do an overflow check again

                if((sndcnt_l >> 3) & 1) {
                    //Decreasing
                    new_freq = shadow_freq - (shadow_freq >> (sndcnt_l & 7));
                } else {
                    //Increasing
                    new_freq = shadow_freq + (shadow_freq >> (sndcnt_l & 7));
                }

                if(new_freq > 2047) {
                    enabled = false;
                }
            }
        }
    }
}

void PulseChannel::tick(u64 late) {
    wave_duty_pos = (wave_duty_pos + 1) % 8;
    const u16 frequency = (2048 - shadow_freq) * 16;

    if(enabled) {
        scheduler.addEvent(frequency_event, frequency - late, [this](u64 late) {
            tick(late);
        });
    }
}

void PulseChannel::restart() {
    enabled = true;

    wave_duty_pos = 0;
    length_timer = (64 - bits::get<0, 6>(sndcnt_h)) * 2;
    current_vol = bits::get<12, 4>(sndcnt_h);
    envelope_timer = bits::get<8, 3>(sndcnt_h) * 8;
    shadow_freq = bits::get<0, 11>(sndcnt_x);
    sweep_timer = bits::get<4, 3>(sndcnt_l) * 4;
    const u16 frequency = (2048 - shadow_freq) * 16;

    //TODO: Sweep overflow check if sweep time is not zero (i.e. is enabled)

    scheduler.removeEvent(frequency_event);
    scheduler.addEvent(frequency_event, frequency, [this](u64 late) {
        tick(late);
    });
}

} //namespace emu