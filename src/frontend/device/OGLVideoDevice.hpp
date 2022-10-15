#pragma once

#include "emulator/device/VideoDevice.hpp"
#include <glad/gl.h>
#include <mutex>
#include <deque>


class OGLVideoDevice final : public emu::VideoDevice {
public:

    OGLVideoDevice();
    ~OGLVideoDevice();

    void clear(u32 color) override;
    void setPixel(int x, int y, u32 color) override;
    void presentFrame() override;

    auto getTextureID() -> GLuint;
    auto getFrameTimes() -> const std::deque<float>& {
        return m_frame_times;
    }

private:

    void createTexture();
    void updateTexture(u32 *pixels);

    u32 *m_present_framebuffer = new u32[sizeof(m_internal_framebuffer)];
    GLuint m_texture_id;
    std::mutex m_update_mutex;
    bool new_frame;

    std::deque<float> m_frame_times;
    std::chrono::time_point<std::chrono::steady_clock> m_start;
};