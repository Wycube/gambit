#pragma once

#include "emulator/device/AudioDevice.hpp"
#include "common/RingBuffer.hpp"
#include <miniaudio.h>


class MAAudioDevice final : public emu::AudioDevice {
public:

    MAAudioDevice();

    void addSample(float sample) override;
    auto full() -> bool override;

// private:

    static void audioCallback(ma_device *device, void *output, const void *input, ma_uint32 frame_count);

    common::ThreadSafeQueue<float, 1024> m_samples;
};