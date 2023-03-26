#include "MAAudioDevice.hpp"
#include "common/Log.hpp"
#include <vector>


MAAudioDevice::MAAudioDevice(void (*callback)(ma_device*, void*, const void*, ma_uint32), void *userdata) {
    LOG_DEBUG("Initializing MAAudioDevice...");

    samples.store(512);
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = 48000;
    config.dataCallback = callback; //audioCallback;
    config.pUserData = userdata; //this;
    config.periodSizeInFrames = 48000 / 64;

    if(ma_device_init(nullptr, &config, &device) != MA_SUCCESS) {
        LOG_FATAL("Could not initalize Miniaudio device!");
    }
}

MAAudioDevice::~MAAudioDevice() {
    ma_device_uninit(&device);
}

void MAAudioDevice::start() {
    if(ma_device_start(&device) != MA_SUCCESS) {
        LOG_FATAL("Unable to start miniaudio device!");
    }
}

void MAAudioDevice::stop() {
    if(ma_device_stop(&device) != MA_SUCCESS) {
        LOG_FATAL("Unable to stop miniaudio device!");
    }
}

void MAAudioDevice::clear() {
    samples_l.clear();
    samples_r.clear();
}

void MAAudioDevice::pushSample(float left, float right) {
    samples_l.push(left);
    samples_r.push(right);
}

auto MAAudioDevice::full() -> bool {
    return samples_l.size() == samples_l.capacity();
}

void MAAudioDevice::setSampleRate(int resolution) {
    samples.store(512 << resolution);
    sample_counter.store(0);
}

void MAAudioDevice::resample(float *dst, size_t channel_size) {
    size_t frame_size = samples.load();
    std::vector<float> frame(frame_size * 2);

    samples_l.pop_many(frame.data(), frame_size);
    samples_r.pop_many(frame.data() + frame_size, frame_size);

    for(size_t i = 0; i < channel_size; i++) {
        size_t index = (size_t)((i / (float)channel_size) * frame_size);
        dst[i] = frame[index];
        dst[i + channel_size] = frame[index + frame_size];
    }
}

void MAAudioDevice::audioCallback(ma_device *device, void *output, const void *input, ma_uint32 frame_count) {
    MAAudioDevice *audio_device = reinterpret_cast<MAAudioDevice*>(device->pUserData);
    float *f_output = reinterpret_cast<float*>(output);

    //Signal the emu thread


    //Record metrics for frontend
    // frontend->m_audio_buffer_mutex.lock();
    // frontend->m_audio_buffer_size[frontend->m_audio_buffer_size_start] = audio_device.m_samples_l.size();
    // frontend->m_audio_buffer_size_start = (frontend->m_audio_buffer_size_start + 1) % 100;
    // frontend->m_audio_buffer_mutex.unlock();

    if(audio_device->samples_l.size() < 1024) {
        LOG_ERROR("Not enough samples for audio callback");
        return;
    }

    //Resample 512 samples to 750
    float samples[2048];
    audio_device->samples_l.pop_many(samples, 1024);
    audio_device->samples_r.pop_many(samples + 1024, 1024);
    // frontend->m_audio_buffer_mutex.lock();
    // std::memcpy(frontend->m_audio_samples, samples, 512 * sizeof(float));
    // frontend->m_audio_buffer_mutex.unlock();

    for(size_t i = 0; i < frame_count; i++) {
        size_t index = (size_t)((i / (float)frame_count) * 1024);
        f_output[i * 2 + 0] = samples[index];
        f_output[i * 2 + 1] = samples[index + 1024];
    }
}