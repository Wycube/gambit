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

    m_update_event = m_core.scheduler.generateHandle();
    m_core.scheduler.addEvent(m_update_event, [this](u64 late) { update(late); }, 512);
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
        case 0x81 : m_sndcnt_l = (m_sndcnt_l & 0x00FF) | (value << 8); break;
        case 0x82 : m_sndcnt_h = (m_sndcnt_h & 0xFF00) | (value & 0x0F); break;
        case 0x83 : m_sndcnt_h = (m_sndcnt_h & 0x00FF) | (value << 8); break;
        case 0x84 : m_sndcnt_x = value & 0x80; break;

        case 0xA0 :
        case 0xA1 :
        case 0xA2 :
        case 0xA3 : m_fifo_a.push(static_cast<s8>(value)); break;

        case 0xA4 :
        case 0xA5 :
        case 0xA6 :
        case 0xA7 : m_fifo_b.push(static_cast<s8>(value)); break;
    }
}

void APU::onTimerOverflow(int timer) {
    if(bits::get_bit<10>(m_sndcnt_h) == timer) {
        if(!m_fifo_a.empty()) {
            m_fifo_sample_a = m_fifo_a.front();
            m_fifo_a.pop();
        }

        if(m_fifo_a.size() <= 4) {
            m_core.dma.onTimerOverflow(0);
        }
    }

    if(bits::get_bit<14>(m_sndcnt_h) == timer) {
        if(!m_fifo_b.empty()) {
            m_fifo_sample_b = m_fifo_b.front();
            m_fifo_b.pop();
        }

        if(m_fifo_b.size() <= 4) {
            m_core.dma.onTimerOverflow(1);
        }
    }
}

auto APU::isTimerSelected(int timer) -> bool {
    return bits::get_bit<10>(m_sndcnt_h) == timer || bits::get_bit<14>(m_sndcnt_h) == timer;
}

void APU::update(u64 late) {
    m_pulse1.step();
    m_pulse2.step();
    m_wave.step();
    m_noise.step();

    s16 sample = 0;
    sample += m_pulse1.amplitude();
    sample += m_pulse2.amplitude();
    sample += m_wave.amplitude();
    sample += m_noise.amplitude();
    sample *= 1 + (m_sndcnt_l & 7);
    sample += m_fifo_sample_a << bits::get_bit<2>(m_sndcnt_l);
    sample += m_fifo_sample_b << bits::get_bit<3>(m_sndcnt_l);

    m_core.audio_device.addSample(sample / (float)0x800);

    m_core.scheduler.addEvent(m_update_event, [this](u64 late) {
        update(late);
    }, 512 - late);
}

} //namespace emu