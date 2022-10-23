#pragma once


namespace emu {

class AudioDevice {
public:

    virtual void addSample(float sample) = 0;
    virtual auto full() -> bool = 0;
};

class NullAudioDevice : public AudioDevice {
public:

    void addSample(float sample) override { }
    auto full() -> bool override { return false; }
};

} //namespace emu