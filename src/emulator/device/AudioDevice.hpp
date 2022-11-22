#pragma once


namespace emu {

class AudioDevice {
public:

    virtual void pushSample(float left, float right) = 0;
    virtual auto full() -> bool = 0;
    virtual void setSampleRate(int resolution) = 0;
};

} //namespace emu