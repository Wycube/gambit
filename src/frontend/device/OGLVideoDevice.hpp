#pragma once

#include "emulator/device/VideoDevice.hpp"
#include "common/Buffer.hpp"
#include <glad/gl.h>
#include <mutex>
#include <deque>


class OGLVideoDevice final : public emu::VideoDevice {
public:

    OGLVideoDevice();
    ~OGLVideoDevice();

    void clear(u32 color);
    void setPixel(int x, int y, u32 color) override;
    void setLine(int y, const u32 *colors) override;
    void presentFrame() override;

    auto getTextureID() -> GLuint;
    auto getFrameTimes() -> const common::ThreadSafeRingBuffer<float, 100>& {
        return frame_times;
    }

private:

    void createTexture();
    void updateTexture(u32 *pixels);

    u32 *internal_framebuffer = new u32[240 * 160];
    u32 *present_framebuffer = new u32[240 * 160];
    GLuint texture_id;
    std::mutex update_mutex;
    bool new_frame;

    common::ThreadSafeRingBuffer<float, 100> frame_times;
    std::chrono::time_point<std::chrono::steady_clock> start;
};