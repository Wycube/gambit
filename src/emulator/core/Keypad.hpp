#pragma once

#include "emulator/device/InputDevice.hpp"
#include "common/Types.hpp"


namespace emu {

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

    Keypad(InputDevice &input_device);

    void reset();

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

    void checkForInterrupt();

private:

    InputDevice &m_input_device;
    u16 m_keyinput;
    u16 m_keycnt;
};

} //namespace emu