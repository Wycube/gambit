#include "APU.hpp"
#include "emulator/core/GBA.hpp"
#include "common/Log.hpp"
#include <algorithm>


namespace emu {

static constexpr s8 WAVE_DUTY[4][8] = {
    {1, -1, -1, -1, -1, -1, -1, -1},
    {1,  1, -1, -1, -1, -1, -1, -1},
    {1,  1,  1,  1, -1, -1, -1, -1},
    {1,  1,  1,  1,  1,  1, -1, -1}
};

APU::APU(GBA &core) : m_core(core), m_pulse1(m_core.scheduler), m_pulse2(m_core.scheduler) {
    reset();
}

void APU::reset() {
    m_pulse1.reset();
    m_pulse2.reset();

    m_update_event = m_core.scheduler.generateHandle();
    m_core.scheduler.addEvent(m_update_event, [this](u64 a, u32 b) {
        update(a, b);
    }, 512);
}

auto APU::read(u32 address) -> u8 {
    if(address >= 0x60 && address <= 0x65) {
        return m_pulse1.read(address);
    }
    if(address >= 0x68 && address <= 0x6D) {
        return m_pulse2.read(address);
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
    if(address == 0x82) {
        m_sndcnt_h = (m_sndcnt_h & 0xFF00) | value;
    }
    if(address == 0x83) {
        m_sndcnt_h = (m_sndcnt_h & 0x00FF) | (value << 8);
    }

    switch(address) {
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
            m_fifo_sample_a = m_fifo_a.front() / 128.0f;
            m_fifo_a.pop();
        }

        if(m_fifo_a.size() <= 4) {
            m_core.dma.onTimerOverflow(0);
        }
    }

    if(bits::get_bit<14>(m_sndcnt_h) == timer) {
        if(!m_fifo_b.empty()) {
            m_fifo_sample_b = m_fifo_b.front() / 128.0f;
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

void APU::update(u64 current, u32 late) {
    m_pulse1.step();
    m_pulse2.step();

    float sample = (m_pulse1.amplitude() + m_pulse2.amplitude()) / 30.0f + m_fifo_sample_a + m_fifo_sample_b;
    m_core.audio_device.addSample(sample);

    m_core.scheduler.addEvent(m_update_event, [this](u64 a, u32 b) {
        update(a, b);
    }, 512 - late);
}

} //namespace emu