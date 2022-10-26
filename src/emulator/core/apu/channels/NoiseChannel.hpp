#pragma once

#include "common/Types.hpp"


namespace emu {

class NoiseChannel {
public:

    void reset();

    auto read(u32 address) -> u8;
    void write(u32 address, u8 value);

private:

    u16 m_snd4cnt_l; //NR41, NR42
    u16 m_snd4cnt_h; //NR43, NR44
};

} //namespace emu