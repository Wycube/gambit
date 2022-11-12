#pragma once

#include "common/Types.hpp"
#include <functional>


namespace emu {

class InputDevice {
public:

    //Returns the keys in the same format as the KEYINPUT register (i.e. 0 is pressed and only bits 0-9 are used).
    virtual auto getKeys() -> u16 = 0;
    void onInput(const std::function<void()> &callback) { m_callback = callback; }

protected:

    std::function<void()> m_callback;
};

class NullInputDevice final : public InputDevice {
public:

    auto getKeys() -> u16 override {
        return 0x3FF;
    }
};

} //namespace emu