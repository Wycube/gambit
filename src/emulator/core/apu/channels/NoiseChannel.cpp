#include "NoiseChannel.hpp"
#include "common/Log.hpp"


namespace emu {

NoiseChannel::NoiseChannel(Scheduler &scheduler) : m_scheduler(scheduler) { }

void NoiseChannel::reset() {
    m_snd4cnt_l = 0;
    m_snd4cnt_h = 0;
    m_enabled = false;

    m_frequency_event = m_scheduler.generateHandle();
    LOG_DEBUG("Noise channel has event handle: {}", m_frequency_event);
}

auto NoiseChannel::read(u32 address) -> u8 {
    switch(address) {
        case 0x79 : return m_snd4cnt_l >> 8;

        case 0x7C : return m_snd4cnt_h & 0xFF;
        case 0x7D : return (m_snd4cnt_h >> 8) & 0x40;
    }

    return 0;
}

void NoiseChannel::write(u32 address, u8 value) {
    switch(address) {
        case 0x78 : m_snd4cnt_l = (m_snd4cnt_l & 0xFF00) | value; break;
        case 0x79 : m_snd4cnt_l = (m_snd4cnt_l & 0x00FF) | (value << 8); break;

        case 0x7C : m_snd4cnt_h = (m_snd4cnt_h & 0xFF00) | value; break;
        case 0x7D : m_snd4cnt_h = (m_snd4cnt_h & 0x00FF) | (value << 8); 
            if(value >> 7) {
                restart();
            }
            break;
    }
}

void NoiseChannel::step() {
    if(bits::get_bit<14>(m_snd4cnt_h) && m_length_timer > 0 && --m_length_timer == 0) {
        m_enabled = false;
    }

    if(bits::get<8, 3>(m_snd4cnt_l) != 0 && m_envelope_timer > 0 && --m_envelope_timer == 0) {
        const u8 envelope_period = bits::get<8, 3>(m_snd4cnt_l) == 0 ? 8 : bits::get<8, 3>(m_snd4cnt_l);
        m_envelope_timer = envelope_period * 512;

        if(bits::get_bit<11>(m_snd4cnt_l)) {
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
}

auto NoiseChannel::amplitude() -> s8 {
    if(m_enabled) {
        return m_high ? m_current_vol : -m_current_vol;
    }

    return 0;
}

void NoiseChannel::tick(u64 late) {
    bool carry = m_lfsr & 1;
    m_high = carry;
    m_lfsr >>= 1;

    if(carry) {
        m_lfsr ^= bits::get_bit<3>(m_snd4cnt_h) ? 0x60 : 0x6000;
    }

    u8 r = bits::get<0, 4>(m_snd4cnt_h);
    const u32 frequency = (r == 0 ? 16 : r * 32) << (bits::get<4, 4>(m_snd4cnt_h) + 1);

    m_scheduler.addEvent(m_frequency_event, [this](u64 late) {
        tick(late);
    }, frequency);
}

void NoiseChannel::restart() {
    m_enabled = true;

    m_length_timer = (64 - bits::get<0, 6>(m_snd4cnt_l)) * 128;
    m_current_vol = bits::get<12, 4>(m_snd4cnt_l);
    m_envelope_timer = bits::get<8, 3>(m_snd4cnt_l) * 512;
    m_lfsr = bits::get_bit<3>(m_snd4cnt_h) ? 0x40 : 0x4000;
    u8 r = bits::get<0, 4>(m_snd4cnt_h);
    u32 frequency = 32 << (bits::get<4, 4>(m_snd4cnt_h) + 1);
    frequency = r == 0 ? frequency / 2 : frequency * r;

    m_scheduler.removeEvent(m_frequency_event);
    m_scheduler.addEvent(m_frequency_event, [this](u64 late) {
        tick(late);
    }, frequency);
}

} //namespace emu