#include "WaveChannel.hpp"
#include "common/Bits.hpp"
#include "common/Log.hpp"
#include <cstring>

static constexpr u8 VOLUMES[4] = {0, 4, 2, 1};


namespace emu {

WaveChannel::WaveChannel(Scheduler &scheduler) : scheduler(scheduler) {
    sample_event = scheduler.registerEvent([this](u64 late) { sample(late); });
    LOG_DEBUG("Wave channel has event handle: {}", sample_event);
}

void WaveChannel::reset() {
    snd3cnt_l = 0;
    snd3cnt_h = 0;
    snd3cnt_x = 0;
    std::memset(wave_ram, 0, sizeof(wave_ram));
    enabled = false;
}

auto WaveChannel::read(u32 address) -> u8 {
    switch(address) {
        case 0x70 : return snd3cnt_l & 0xFF;
        case 0x73 : return (snd3cnt_h >> 8) & 0xE0;
        case 0x75 : return (snd3cnt_x >> 8) & 0x40;
    }

    if(address >= 0x90 && address <= 0x9F) {
        return wave_ram[(address - 0x90) + bits::get_bit<6>(~snd3cnt_l) * 16];
    }

    return 0;
}

void WaveChannel::write(u32 address, u8 value) {
    switch(address) {
        case 0x70 : snd3cnt_l = value & 0xE0; break;

        case 0x72 : snd3cnt_h = (snd3cnt_h & 0xFF00) | value; break;
        case 0x73 : snd3cnt_h = (snd3cnt_h & 0x00FF) | (value << 8); break;
        case 0x74 : snd3cnt_x = (snd3cnt_x & 0xFF00) | value; break;
        case 0x75 : snd3cnt_x = (snd3cnt_x & 0x00FF) | (value << 8); 
            if(value >> 7) {
                restart();
            }
            break;
    }

    if(address >= 0x90 && address <= 0x9F) {
        wave_ram[(address - 0x90) + bits::get_bit<6>(~snd3cnt_l) * 16] = value;
    }
}

auto WaveChannel::amplitude() -> u8 {
    if(enabled && !bits::get_bit<7>(snd3cnt_l)) {
        u8 wave_index = wave_pos;
        u8 volume = VOLUMES[bits::get<13, 2>(snd3cnt_h)];

        if(bits::get_bit<15>(snd3cnt_h)) {
            volume = 3;
        }

        if(bits::get_bit<6>(snd3cnt_l) && !bits::get_bit<5>(snd3cnt_l)) {
            wave_index += 32;
        }

        if(wave_index % 2 == 0) {
            return ((wave_ram[wave_index / 2] >> 4) * volume) / 4;
        } else {
            return ((wave_ram[wave_index / 2] & 0xF) * volume) / 4;
        }
    } else {
        return 0;
    }
}

void WaveChannel::step() {
    if(bits::get_bit<14>(snd3cnt_x) && length_timer > 0 && --length_timer == 0) {
        enabled = false;
    }
}

void WaveChannel::sample(u64 late) {
    const u32 sample_rate = (2048 - bits::get<0, 11>(snd3cnt_x)) * 8;
    wave_pos = (wave_pos + 1) % (bits::get_bit<5>(snd3cnt_l) ? 64 : 32);

    scheduler.addEvent(sample_event, sample_rate - late);
}

void WaveChannel::restart() {
    enabled = true;

    length_timer = (256 - (snd3cnt_l & 0xFF)) * 2;
    wave_pos = 0;
    const u32 sample_rate = (2048 - bits::get<0, 11>(snd3cnt_x)) * 8;

    scheduler.removeEvent(sample_event);
    scheduler.addEvent(sample_event, sample_rate);
}

} //namespace emu