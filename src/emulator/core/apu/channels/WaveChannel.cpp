#include "WaveChannel.hpp"
#include "common/Bits.hpp"
#include <cstring>


namespace emu {

void WaveChannel::reset() {
    m_snd3cnt_l = 0;
    m_snd3cnt_h = 0;
    m_snd3cnt_x = 0;
    memset(m_wave_ram, 0, sizeof(m_wave_ram));
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
        case 0x75 : m_snd3cnt_x = (m_snd3cnt_x & 0x00FF) | (value << 8); break;
    }

    if(address >= 0x90 && address <= 0x9F) {
        m_wave_ram[(address - 0x90) + bits::get_bit<6>(~m_snd3cnt_l) * 16] = value;
    }
}

} //namespace emu