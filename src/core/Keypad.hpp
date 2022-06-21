#pragma once

#include "common/Types.hpp"


namespace emu {

enum KeypadInput : u16 {
    BUTTON_A = 1 << 0,
    BUTTON_B = 1 << 1,
    SELECT   = 1 << 2,
    START    = 1 << 3,
    RIGHT    = 1 << 4,
    LEFT     = 1 << 5,
    UP       = 1 << 6,
    DOWN     = 1 << 7,
    BUTTON_R = 1 << 8,
    BUTTON_L = 1 << 9
};

class Keypad {
public:

    Keypad();

    void reset();

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

    void set_keys(u16 keys);
    auto get_keys() -> u16;

private:

    u16 m_keyinput;
    u16 m_keycnt;
};

} //namespace emu