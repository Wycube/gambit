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

PulseChannel::PulseChannel(Scheduler &scheduler) : m_scheduler(scheduler) { }

void PulseChannel::reset() {
    m_sndcnt_h = 0;
    m_sndcnt_l = 0;
    m_sndcnt_x = 0;
    m_enabled = false;
    m_frequency_event = m_scheduler.generateHandle();
    LOG_DEBUG("Pulse channel has event handle: {}", m_frequency_event);
}

auto PulseChannel::read(u32 address) -> u8 {
    switch(address) {
        case 0x60 : return m_sndcnt_l;

        case 0x62 :
        case 0x68 : return m_sndcnt_h & 0xC0;
        case 0x63 :
        case 0x69 : return m_sndcnt_h >> 8;

        case 0x65 :
        case 0x6D : return (m_sndcnt_x >> 8) & 0x40;
    }

    return 0;
}

void PulseChannel::write(u32 address, u8 value) {
        switch(address) {
        case 0x60 : m_sndcnt_l = value & 0x7F; break;

        case 0x62 :
        case 0x68 : m_sndcnt_h = (m_sndcnt_h & 0xFF00) | value; break;
        case 0x63 :
        case 0x69 : m_sndcnt_h = (m_sndcnt_h & 0x00FF) | (value << 8); break;

        case 0x64 :
        case 0x6C : m_sndcnt_x = (m_sndcnt_x & 0xFF00) | value; break;
        case 0x65 :
        case 0x6D : m_sndcnt_x = (m_sndcnt_x & 0x00FF) | (value << 8); 
            if(value >> 7) {
                restart();
            }
            break;
    }
}

auto PulseChannel::amplitude() -> s8 {
    if(m_enabled) {
        return WAVE_DUTY[bits::get<6, 2>(m_sndcnt_h)][m_wave_duty_pos] * m_current_vol;
    }

    return 0;
}

void PulseChannel::step() {
    if(bits::get_bit<14>(m_sndcnt_x) && m_length_timer > 0 && --m_length_timer == 0) {
        m_enabled = false;
    }

    if(bits::get<8, 3>(m_sndcnt_h) != 0 && m_envelope_timer > 0 && --m_envelope_timer == 0) {
        const u8 envelope_period = bits::get<8, 3>(m_sndcnt_h) == 0 ? 8 : bits::get<8, 3>(m_sndcnt_h);
        m_envelope_timer = envelope_period * 512;

        if(bits::get_bit<11>(m_sndcnt_h)) {
            //Increase
            if(m_current_vol < 15) {
                m_current_vol++;
            }
        } else {
            //Decrease
            if(m_current_vol > 0) {
                m_current_vol--;
            }
        }
    }

    if(m_sweep_timer > 0 && --m_sweep_timer == 0) {
        m_sweep_timer = bits::get<4, 3>(m_sndcnt_l) * 256;

        if((m_sndcnt_l & 7) != 0 && bits::get<4, 3>(m_sndcnt_l) != 0) {
            u16 new_freq;

            if((m_sndcnt_l >> 3) & 1) {
                //Decreasing
                new_freq = m_shadow_freq - (m_shadow_freq << (m_sndcnt_l & 7));
            } else {
                //Increasing
                new_freq = m_shadow_freq + (m_shadow_freq << (m_sndcnt_l & 7));
            }

            if(new_freq > 2047) {
                m_enabled = false;
            } else {
                m_shadow_freq = new_freq;
                m_sndcnt_x = (m_sndcnt_x & ~0x7FF) | new_freq;

                //Do an overflow check again

                if((m_sndcnt_l >> 3) & 1) {
                    //Decreasing
                    new_freq = m_shadow_freq - (m_shadow_freq << (m_sndcnt_l & 7));
                } else {
                    //Increasing
                    new_freq = m_shadow_freq + (m_shadow_freq << (m_sndcnt_l & 7));
                }

                if(new_freq > 2047) {
                    m_enabled = false;
                }
            }
        }
    }
}

void PulseChannel::tick(u64 late) {
    m_wave_duty_pos = (m_wave_duty_pos + 1) % 8;
    const u16 frequency = (2048 - m_shadow_freq) * 16;

    if(m_enabled) {
        m_scheduler.addEvent(m_frequency_event, [this](u64 late) {
            tick(late);
        }, frequency - late);
    }
}

void PulseChannel::restart() {
    m_enabled = true;

    m_wave_duty_pos = 0;
    m_length_timer = (64 - bits::get<0, 6>(m_sndcnt_h)) * 128;
    m_current_vol = bits::get<12, 4>(m_sndcnt_h);
    m_envelope_timer = bits::get<8, 3>(m_sndcnt_h) * 512;
    m_shadow_freq = bits::get<0, 11>(m_sndcnt_x);
    m_sweep_timer = bits::get<4, 3>(m_sndcnt_l) * 256;
    const u16 frequency = (2048 - m_shadow_freq) * 16;

    //TODO: Sweep overflow check if sweep time is not zero (i.e. is enabled)

    m_scheduler.removeEvent(m_frequency_event);
    m_scheduler.addEvent(m_frequency_event, [this](u64 late) {
        tick(late);
    }, frequency);
}

} //namespace emu