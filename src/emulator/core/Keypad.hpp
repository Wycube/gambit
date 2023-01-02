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

    explicit Keypad(GBA &core);

    void reset();

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

private:

    GBA &core;
    u16 keycnt;
    std::mutex keycnt_mutex;

    void checkForInterrupt();
};

} //namespace emu