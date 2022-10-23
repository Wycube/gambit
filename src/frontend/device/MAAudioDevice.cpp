#include "MAAudioDevice.hpp"


MAAudioDevice::MAAudioDevice() {
    // ma_device_config config = ma_device_config_init(ma_device_type_playback);
    // config.playback.format = ma_format_f32;
    // config.playback.channels = 2;
    // config.sampleRate = 48000;
    // config.dataCallback = audioCallback;
    // config.pUserData = this;
    // config.periodSizeInFrames = 48000 / 64;
}

void MAAudioDevice::addSample(float sample) {
    m_samples.push(sample);
}

auto MAAudioDevice::full() -> bool {
    return m_samples.size() == 1024;
}

void MAAudioDevice::audioCallback(ma_device *device, void *output, const void *input, ma_uint32 frame_count) {

}