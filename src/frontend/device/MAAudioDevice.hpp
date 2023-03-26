#pragma once

#include "emulator/device/AudioDevice.hpp"
#include "common/Buffer.hpp"
#define MA_NO_ENCODING
#define MA_NO_DECODING
#include <miniaudio.h>
#include <atomic>


class MAAudioDevice final : public emu::AudioDevice {
public:

    MAAudioDevice(void (*callback)(ma_device*, void*, const void*, ma_uint32), void *userdata);
    ~MAAudioDevice();

    void start();
    void stop();

    void clear();
    void pushSample(float left, float right) override;
    auto full() -> bool override;
    void setSampleRate(int resolution) override;
    void resample(float *dst, size_t size);

// private:

    static void audioCallback(ma_device *device, void *output, const void *input, ma_uint32 frame_count);
    
    ma_device device;
    common::ThreadSafeRingBuffer<float, 8192> samples_l;
    common::ThreadSafeRingBuffer<float, 8192> samples_r;
    std::atomic<int> samples;
    std::atomic<int> sample_counter;
};