#include "APU.hpp"
#include "emulator/core/GBA.hpp"
#include "common/Log.hpp"
#include <algorithm>


namespace emu {

APU::APU(GBA &core) : core(core), pulse1(core.scheduler), pulse2(core.scheduler), wave(core.scheduler), noise(core.scheduler) {
    step_event = core.scheduler.registerEvent([this](u64 late) { step(late); });
    sample_event = core.scheduler.registerEvent([this](u64 late) { sample(late); });
    LOG_DEBUG("APU has event handle: {} and {}", step_event, sample_event);

    reset();
}

void APU::reset() {
    pulse1.reset();
    pulse2.reset();
    wave.reset();
    noise.reset();

    sndcnt_l = 0;
    sndcnt_h = 0xE;
    sndcnt_x = 0;
    sndbias = 0x200;
    core.audio_device.setSampleRate(bits::get<14, 2>(sndbias));

    core.scheduler.addEvent(step_event, 32768);
    core.scheduler.addEvent(sample_event, 512);
}

auto APU::read(u32 address) -> u8 {
    if(address >= 0x60 && address <= 0x65) {
        return pulse1.read(address);
    }
    if(address >= 0x68 && address <= 0x6D) {
        return pulse2.read(address);
    }
    if((address >= 0x70 && address <= 0x77) || (address >= 0x90 && address <= 0x9F)) {
        return wave.read(address);
    }
    if(address >= 0x78 && address <= 0x7F) {
        return noise.read(address);
    }

    switch(address) {
        case 0x80 : return sndcnt_l & 0xFF;
        case 0x81 : return sndcnt_l >> 8;
        case 0x82 : return sndcnt_h & 0xFF;
        case 0x83 : return (sndcnt_h >> 8) & 0x77;
        case 0x84 : return sndcnt_x;
        case 0x88 : return sndbias & 0xFF;
        case 0x89 : return sndbias >> 8;
    }

    return 0;
}

void APU::write(u32 address, u8 value) {
    if(address >= 0x60 && address <= 0x65) {
        pulse1.write(address, value);
    }
    if(address >= 0x68 && address <= 0x6D) {
        pulse2.write(address, value);
    }
    if((address >= 0x70 && address <= 0x77) || (address >= 0x90 && address <= 0x9F)) {
        wave.write(address, value);
    }
    if(address >= 0x78 && address <= 0x7F) {
        noise.write(address, value);
    }

    switch(address) {
        case 0x80 : sndcnt_l = (sndcnt_l & 0xFF00) | (value & 0x77); break;
        case 0x81 : sndcnt_l = (sndcnt_l & 0x00FF) | value << 8; break;
        case 0x82 : sndcnt_h = (sndcnt_h & 0xFF00) | (value & 0x0F); break;
        case 0x83 : sndcnt_h = (sndcnt_h & 0x00FF) | value << 8; 
            if(bits::get_bit<3>(value)) {
                fifo_a.clear();
            }
            if(bits::get_bit<7>(value)) {
                fifo_b.clear();
            }
            break;
        case 0x84 : sndcnt_x = value & 0x80; break;
        case 0x88 : sndbias = (sndbias & 0xFF00) | (value & ~1); break;
        case 0x89 : 
            sndbias = (sndbias & 0x00FF) | (value & 0xC3) << 8;
            core.audio_device.setSampleRate(bits::get<14, 2>(sndbias));
            LOG_DEBUG("Sampling rate {}KHz selected", 32 << bits::get<14, 2>(sndbias));
            break;

        case 0xA0 :
        case 0xA1 :
        case 0xA2 :
        case 0xA3 : fifo_a.push_back(static_cast<s8>(value)); break;

        case 0xA4 :
        case 0xA5 :
        case 0xA6 :
        case 0xA7 : fifo_b.push_back(static_cast<s8>(value)); break;
    }
}

void APU::onTimerOverflow(int timer) {
    if(bits::get_bit<10>(sndcnt_h) == timer) {
        if(!fifo_a.empty()) {
            fifo_sample_a = fifo_a.front();
            fifo_a.pop_front();
        }

        if(fifo_a.size() <= 4) {
            core.dma.onTimerOverflow(0);
        }
    }

    if(bits::get_bit<14>(sndcnt_h) == timer) {
        if(!fifo_b.empty()) {
            fifo_sample_b = fifo_b.front();
            fifo_b.pop_front();
        }

        if(fifo_b.size() <= 4) {
            core.dma.onTimerOverflow(1);
        }
    }
}

auto APU::isTimerSelected(int timer) -> bool {
    return bits::get_bit<10>(sndcnt_h) == timer || bits::get_bit<14>(sndcnt_h) == timer;
}

void APU::step(u64 late) {
    pulse1.step();
    pulse2.step();
    wave.step();
    noise.step();

    core.scheduler.addEvent(step_event, 32768 - late);
}

void APU::sample(u64 late) {
    s16 sample_l = 0;
    s16 sample_r = 0;
    if(bits::get_bit<8>(sndcnt_l)) sample_r += pulse1.amplitude();
    if(bits::get_bit<9>(sndcnt_l)) sample_r += pulse2.amplitude();
    if(bits::get_bit<10>(sndcnt_l)) sample_r += wave.amplitude();
    if(bits::get_bit<11>(sndcnt_l)) sample_r += noise.amplitude();
    if(bits::get_bit<12>(sndcnt_l)) sample_l += pulse1.amplitude();
    if(bits::get_bit<13>(sndcnt_l)) sample_l += pulse2.amplitude();
    if(bits::get_bit<14>(sndcnt_l)) sample_l += wave.amplitude();
    if(bits::get_bit<15>(sndcnt_l)) sample_l += noise.amplitude();
    
    sample_r *= 1 + (sndcnt_l & 7);
    sample_l *= 1 + ((sndcnt_l >> 4) & 7);
    int volume_shift = 2 - bits::get<0, 2>(sndcnt_h);
    if(volume_shift != 3) {
        sample_r >>= volume_shift;
        sample_l >>= volume_shift;
    }

    if(bits::get_bit<8>(sndcnt_h)) sample_r += fifo_sample_a << bits::get_bit<2>(sndcnt_h);
    if(bits::get_bit<9>(sndcnt_h)) sample_l += fifo_sample_a << bits::get_bit<2>(sndcnt_h);
    if(bits::get_bit<12>(sndcnt_h)) sample_r += fifo_sample_b << bits::get_bit<3>(sndcnt_h);
    if(bits::get_bit<13>(sndcnt_h)) sample_l += fifo_sample_b << bits::get_bit<3>(sndcnt_h);
    sample_r += bits::get<0, 9>(sndbias);
    sample_l += bits::get<0, 9>(sndbias);

    core.audio_device.pushSample(sample_l / (float)0x800, sample_r / (float)0x800);

    core.scheduler.addEvent(sample_event, (512 >> bits::get<14, 2>(sndbias)) - late);
}

} //namespace emu