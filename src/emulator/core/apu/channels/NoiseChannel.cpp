#include "NoiseChannel.hpp"
#include "common/Bits.hpp"
#include "common/Log.hpp"


namespace emu {

NoiseChannel::NoiseChannel(Scheduler &scheduler) : scheduler(scheduler) {
    frequency_event = scheduler.registerEvent([this](u64 late) { tick(late); });
    LOG_DEBUG("Noise channel has event handle: {}", frequency_event);
}

void NoiseChannel::reset() {
    snd4cnt_l = 0;
    snd4cnt_h = 0;
    enabled = false;
}

void NoiseChannel::serialize(std::ofstream &file) {
    file.write(reinterpret_cast<const char*>(&snd4cnt_l), sizeof(snd4cnt_l));
    file.write(reinterpret_cast<const char*>(&snd4cnt_h), sizeof(snd4cnt_h));
    file.write(reinterpret_cast<const char*>(&enabled), sizeof(enabled));
    file.write(reinterpret_cast<const char*>(&current_vol), sizeof(current_vol));
    file.write(reinterpret_cast<const char*>(&envelope_timer), sizeof(envelope_timer));
    file.write(reinterpret_cast<const char*>(&length_timer), sizeof(length_timer));
    file.write(reinterpret_cast<const char*>(&lfsr), sizeof(lfsr));
    file.write(reinterpret_cast<const char*>(&high), sizeof(high));
}

void NoiseChannel::deserialize(std::ifstream &file) {
    file.read(reinterpret_cast<char*>(&snd4cnt_l), sizeof(snd4cnt_l));
    file.read(reinterpret_cast<char*>(&snd4cnt_h), sizeof(snd4cnt_h));
    file.read(reinterpret_cast<char*>(&enabled), sizeof(enabled));
    file.read(reinterpret_cast<char*>(&current_vol), sizeof(current_vol));
    file.read(reinterpret_cast<char*>(&envelope_timer), sizeof(envelope_timer));
    file.read(reinterpret_cast<char*>(&length_timer), sizeof(length_timer));
    file.read(reinterpret_cast<char*>(&lfsr), sizeof(lfsr));
    file.read(reinterpret_cast<char*>(&high), sizeof(high));
}

auto NoiseChannel::read(u32 address) -> u8 {
    switch(address) {
        case 0x79 : return snd4cnt_l >> 8;

        case 0x7C : return snd4cnt_h & 0xFF;
        case 0x7D : return (snd4cnt_h >> 8) & 0x40;
    }

    return 0;
}

void NoiseChannel::write(u32 address, u8 value) {
    switch(address) {
        case 0x78 : snd4cnt_l = (snd4cnt_l & 0xFF00) | value; break;
        case 0x79 : snd4cnt_l = (snd4cnt_l & 0x00FF) | (value << 8); break;

        case 0x7C : snd4cnt_h = (snd4cnt_h & 0xFF00) | value; break;
        case 0x7D : snd4cnt_h = (snd4cnt_h & 0x00FF) | (value << 8); 
            if(value >> 7) {
                restart();
            }
            break;
    }
}

void NoiseChannel::step() {
    if(bits::get_bit<14>(snd4cnt_h) && length_timer > 0 && --length_timer == 0) {
        enabled = false;
    }

    if(envelope_timer > 0 && --envelope_timer == 0) {
        const u8 envelope_period = bits::get<8, 3>(snd4cnt_l) == 0 ? 8 : bits::get<8, 3>(snd4cnt_l);
        envelope_timer = envelope_period * 8;

        if(bits::get_bit<11>(snd4cnt_l)) {
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
}

auto NoiseChannel::amplitude() -> s8 {
    if(enabled && !bits::get<11, 5>(snd4cnt_l)) {
        return high ? current_vol : -current_vol;
    }

    return 0;
}

void NoiseChannel::tick(u64 late) {
    bool carry = lfsr & 1;
    high = carry;
    lfsr >>= 1;

    if(carry) {
        lfsr ^= bits::get_bit<3>(snd4cnt_h) ? 0x60 : 0x6000;
    }

    u8 r = bits::get<0, 4>(snd4cnt_h);
    const u32 frequency = (r == 0 ? 16 : r * 32) << (bits::get<4, 4>(snd4cnt_h) + 1);

    scheduler.addEvent(frequency_event, frequency - late);
}

void NoiseChannel::restart() {
    enabled = true;

    length_timer = (64 - bits::get<0, 6>(snd4cnt_l)) * 2;
    current_vol = bits::get<12, 4>(snd4cnt_l);
    envelope_timer = bits::get<8, 3>(snd4cnt_l) * 8;
    lfsr = bits::get_bit<3>(snd4cnt_h) ? 0x40 : 0x4000;
    u8 r = bits::get<0, 4>(snd4cnt_h);
    u32 frequency = 32 << (bits::get<4, 4>(snd4cnt_h) + 1);
    frequency = r == 0 ? frequency / 2 : frequency * r;

    scheduler.removeEvent(frequency_event);
    scheduler.addEvent(frequency_event, frequency);
}

} //namespace emu