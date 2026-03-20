#pragma once

#include "common/Types.hpp"
#include <mutex>
#include <fstream>


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
    void serialize(std::ofstream &file);
    void deserialize(std::ifstream &file);

    auto read8(u32 address) -> u8;
    void write8(u32 address, u8 value);

private:

    void checkForInterrupt();
    
    GBA &core;
    u16 keycnt;
    std::mutex keycnt_mutex;
};

} //namespace emu