#include "WaveChannel.hpp"
#include "common/Bits.hpp"
#include "common/Log.hpp"
#include <cstring>

static constexpr u8 VOLUMES[4] = {0, 4, 2, 1};


namespace emu {

WaveChannel::WaveChannel(Scheduler &scheduler) : m_scheduler(scheduler) { }

void WaveChannel::reset() {
    m_snd3cnt_l = 0;
    m_snd3cnt_h = 0;
    m_snd3cnt_x = 0;
    std::memset(m_wave_ram, 0, sizeof(m_wave_ram));
    m_enabled = false;

    m_sample_event = m_scheduler.generateHandle();
    LOG_DEBUG("Wave channel has event handle: {}", m_sample_event);
}

auto WaveChannel::read(u32 address) -> u8 {
    switch(address) {
        case 0x70 : return m_snd3cnt_l & 0xFF;
        case 0x73 : return (m_snd3cnt_h >> 8) & 0xE0;
        case 0x75 : return (m_snd3cnt_x >> 8) & 0x40;
    }

    if(address >= 0x90 && address <= 0x9F) {
        return m_wave_ram[(address - 0x90) + bits::get_bit<6>(~m_snd3cnt_l) * 16];
    }

    return 0;
}

void WaveChannel::write(u32 address, u8 value) {
    switch(address) {
        case 0x70 : m_snd3cnt_l = value & 0xE0; break;

        case 0x72 : m_snd3cnt_h = (m_snd3cnt_h & 0xFF00) | value; break;
        case 0x73 : m_snd3cnt_h = (m_snd3cnt_h & 0x00FF) | (value << 8); break;
        case 0x74 : m_snd3cnt_x = (m_snd3cnt_x & 0xFF00) | value; break;
        case 0x75 : m_snd3cnt_x = (m_snd3cnt_x & 0x00FF) | (value << 8); 
            if(value >> 7) {
                restart();
            }
            break;
    }

    if(address >= 0x90 && address <= 0x9F) {
        m_wave_ram[(address - 0x90) + bits::get_bit<6>(~m_snd3cnt_l) * 16] = value;
    }
}

auto WaveChannel::amplitude() -> u8 {
    if(m_enabled && !bits::get_bit<7>(m_snd3cnt_l)) {
        u8 wave_pos = m_wave_pos;
        u8 volume = VOLUMES[bits::get<13, 2>(m_snd3cnt_h)];

        if(bits::get_bit<15>(m_snd3cnt_h)) {
            volume = 3;
        }

        if(bits::get_bit<6>(m_snd3cnt_l) && !bits::get_bit<5>(m_snd3cnt_l)) {
            wave_pos += 32;
        }

        if(wave_pos % 2 == 0) {
            return ((m_wave_ram[wave_pos / 2] >> 4) * volume) / 4;
        } else {
            return ((m_wave_ram[wave_pos / 2] & 0xF) * volume) / 4;
        }
    } else {
        return 0;
    }
}

void WaveChannel::step() {
    if(bits::get_bit<14>(m_snd3cnt_x) && m_length_timer > 0 && --m_length_timer == 0) {
        m_enabled = false;
    }
}

void WaveChannel::sample(u64 late) {
    const u32 sample_rate = (2048 - bits::get<0, 11>(m_snd3cnt_x)) * 8;
    m_wave_pos = (m_wave_pos + 1) % (bits::get_bit<5>(m_snd3cnt_l) ? 64 : 32);

    m_scheduler.addEvent(m_sample_event, [this](u64 late) {
        sample(late);
    }, sample_rate - late);
}

void WaveChannel::restart() {
    m_enabled = true;

    m_length_timer = (256 - (m_snd3cnt_l & 0xFF)) * 2;
    m_wave_pos = 0;
    const u32 sample_rate = (2048 - bits::get<0, 11>(m_snd3cnt_x)) * 8;

    m_scheduler.removeEvent(m_sample_event);
    m_scheduler.addEvent(m_sample_event, [this](u64 late) {
        sample(late);
    }, sample_rate);
}

} //namespace emu