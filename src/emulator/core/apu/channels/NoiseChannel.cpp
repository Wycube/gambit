#include "NoiseChannel.hpp"


namespace emu {

void NoiseChannel::reset() {
    m_snd4cnt_l = 0;
    m_snd4cnt_h = 0;
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
        case 0x7D : m_snd4cnt_h = (m_snd4cnt_h & 0x00FF) | (value << 8); break;
    }
}

} //namespace emu