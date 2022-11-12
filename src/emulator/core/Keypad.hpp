#pragma once

#include "common/Types.hpp"
#include <mutex>


namespace emu {

class GBA;

enum KeypadInput : u16 {
    BUTTON_A,
    BUTTON_B,
    SELECT,
    START,
    RIGHT,
    LEFT,
    UP,
    DOWN ,
    BUTTON_R,
    BUTTON_L
};

class Keypad final {
public:

    Keypad(GBA &core);

    void reset();

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

private:

    GBA &m_core;
    u16 m_keycnt;
    std::mutex m_keycnt_mutex;

    void checkForInterrupt();
};

} //namespace emu