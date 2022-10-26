#pragma once

#include "common/Types.hpp"


namespace emu {

class WaveChannel {
public:

    void reset();

    auto read(u32 address) -> u8;
    void write(u32 address, u8 value);

private:

    u8 m_snd3cnt_l;  //NR30
    u16 m_snd3cnt_h; //NR31, NR32
    u16 m_snd3cnt_x; //NR33, NR34
    u8 m_wave_ram[32];
};

} //namespace emu