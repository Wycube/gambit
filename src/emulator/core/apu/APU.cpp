#include "APU.hpp"
#include "emulator/core/GBA.hpp"
#include "common/Log.hpp"
#include <algorithm>


namespace emu {

APU::APU(GBA &core) : m_core(core), m_pulse1(m_core.scheduler), m_pulse2(m_core.scheduler),
    m_wave(m_core.scheduler), m_noise(m_core.scheduler) {
    reset();
}

void APU::reset() {
    m_pulse1.reset();
    m_pulse2.reset();
    m_wave.reset();
    m_noise.reset();

    m_sndcnt_l = 0;
    m_sndcnt_h = 0xE;
    m_sndcnt_x = 0;
    m_sndbias = 0x200;

    m_update_event = m_core.scheduler.generateHandle();
    m_core.scheduler.addEvent(m_update_event, [this](u64 late) { step(late); }, 32768);
    m_core.scheduler.addEvent(m_update_event, [this](u64 late) { sample(late); }, 512);
    LOG_DEBUG("APU has event handle: {}", m_update_event);
}

auto APU::read(u32 address) -> u8 {
    if(address >= 0x60 && address <= 0x65) {
        return m_pulse1.read(address);
    }
    if(address >= 0x68 && address <= 0x6D) {
        return m_pulse2.read(address);
    }
    if((address >= 0x70 && address <= 0x77) || (address >= 0x90 && address <= 0x9F)) {
        return m_wave.read(address);
    }
    if(address >= 0x78 && address <= 0x7F) {
        return m_noise.read(address);
    }

    switch(address) {
        case 0x80 : return m_sndcnt_l & 0xFF;
        case 0x81 : return m_sndcnt_l >> 8;
        case 0x82 : return m_sndcnt_h & 0xFF;
        case 0x83 : return (m_sndcnt_h >> 8) & 0x77;
        case 0x84 : return m_sndcnt_x;
        case 0x88 : return m_sndbias & 0xFF;
        case 0x89 : return m_sndbias >> 8;
    }

    return 0;
}

void APU::write(u32 address, u8 value) {
    if(address >= 0x60 && address <= 0x65) {
        m_pulse1.write(address, value);
    }
    if(address >= 0x68 && address <= 0x6D) {
        m_pulse2.write(address, value);
    }
    if((address >= 0x70 && address <= 0x77) || (address >= 0x90 && address <= 0x9F)) {
        m_wave.write(address, value);
    }
    if(address >= 0x78 && address <= 0x7F) {
        m_noise.write(address, value);
    }

    switch(address) {
        case 0x80 : m_sndcnt_l = (m_sndcnt_l & 0xFF00) | (value & 0x77); break;
        case 0x81 : m_sndcnt_l = (m_sndcnt_l & 0x00FF) | value << 8; break;
        case 0x82 : m_sndcnt_h = (m_sndcnt_h & 0xFF00) | (value & 0x0F); break;
        case 0x83 : m_sndcnt_h = (m_sndcnt_h & 0x00FF) | value << 8; 
            if(bits::get_bit<3>(value)) {
                m_fifo_a.clear();
            }
            if(bits::get_bit<7>(value)) {
                m_fifo_b.clear();
            }
            break;
        case 0x84 : m_sndcnt_x = value & 0x80; break;
        case 0x88 : m_sndbias = (m_sndbias & 0xFF00) | (value & ~1); break;
        case 0x89 : m_sndbias = (m_sndbias & 0x00FF) | (value & 0xC3) << 8;
            m_core.audio_device.setSampleRate(bits::get<14, 2>(m_sndbias));
            break;

        case 0xA0 :
        case 0xA1 :
        case 0xA2 :
        case 0xA3 : m_fifo_a.push_back(static_cast<s8>(value)); break;

        case 0xA4 :
        case 0xA5 :
        case 0xA6 :
        case 0xA7 : m_fifo_b.push_back(static_cast<s8>(value)); break;
    }
}

void APU::onTimerOverflow(int timer) {
    if(bits::get_bit<10>(m_sndcnt_h) == timer) {
        if(!m_fifo_a.empty()) {
            m_fifo_sample_a = m_fifo_a.front();
            m_fifo_a.pop_front();
        }

        if(m_fifo_a.size() <= 4) {
            m_core.dma.onTimerOverflow(0);
        }
    }

    if(bits::get_bit<14>(m_sndcnt_h) == timer) {
        if(!m_fifo_b.empty()) {
            m_fifo_sample_b = m_fifo_b.front();
            m_fifo_b.pop_front();
        }

        if(m_fifo_b.size() <= 4) {
            m_core.dma.onTimerOverflow(1);
        }
    }
}

auto APU::isTimerSelected(int timer) -> bool {
    return bits::get_bit<10>(m_sndcnt_h) == timer || bits::get_bit<14>(m_sndcnt_h) == timer;
}

void APU::step(u64 late) {
    m_pulse1.step();
    m_pulse2.step();
    m_wave.step();
    m_noise.step();

    m_core.scheduler.addEvent(m_update_event, [this](u64 late) {
        this->step(late);
    }, 32768 - late);
}

void APU::sample(u64 late) {
    s16 sample_l = 0;
    s16 sample_r = 0;
    if(bits::get_bit<8>(m_sndcnt_l)) sample_r += m_pulse1.amplitude();
    if(bits::get_bit<9>(m_sndcnt_l)) sample_r += m_pulse2.amplitude();
    if(bits::get_bit<10>(m_sndcnt_l)) sample_r += m_wave.amplitude();
    if(bits::get_bit<11>(m_sndcnt_l)) sample_r += m_noise.amplitude();
    if(bits::get_bit<12>(m_sndcnt_l)) sample_l += m_pulse1.amplitude();
    if(bits::get_bit<13>(m_sndcnt_l)) sample_l += m_pulse2.amplitude();
    if(bits::get_bit<14>(m_sndcnt_l)) sample_l += m_wave.amplitude();
    if(bits::get_bit<15>(m_sndcnt_l)) sample_l += m_noise.amplitude();
    sample_r *= 1 + (m_sndcnt_l & 7);
    sample_l *= 1 + ((m_sndcnt_l >> 4) & 7);
    int volume_shift = 2 - bits::get<0, 2>(m_sndcnt_h);
    if(volume_shift != 3) sample_r >>= volume_shift; sample_l >>= volume_shift;
    if(bits::get_bit<8>(m_sndcnt_h)) sample_r += m_fifo_sample_a << bits::get_bit<2>(m_sndcnt_h);
    if(bits::get_bit<9>(m_sndcnt_h)) sample_l += m_fifo_sample_a << bits::get_bit<2>(m_sndcnt_h);
    if(bits::get_bit<12>(m_sndcnt_h)) sample_r += m_fifo_sample_b << bits::get_bit<3>(m_sndcnt_h);
    if(bits::get_bit<13>(m_sndcnt_h)) sample_l += m_fifo_sample_b << bits::get_bit<3>(m_sndcnt_h);
    sample_r += bits::get<0, 9>(m_sndbias);
    sample_l += bits::get<0, 9>(m_sndbias);

    m_core.audio_device.pushSample(sample_l / (float)0x800, sample_r / (float)0x800);

    m_core.scheduler.addEvent(m_update_event, [this](u64 late) {
        this->sample(late);
    }, (512 >> bits::get<14, 2>(m_sndbias)) - late);
}

} //namespace emu