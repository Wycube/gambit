#pragma once

#include "emulator/device/VideoDevice.hpp"
#include <glad/gl.h>
#include <mutex>


class OGLVideoDevice final : public emu::VideoDevice {
public:

    OGLVideoDevice();
    ~OGLVideoDevice();

    void clear(u32 color) override;
    void setPixel(int x, int y, u32 color) override;
    void presentFrame() override;

    auto getTextureID() -> GLuint;

private:

    void createTexture();
    void updateTexture(u32 *pixels);

    u32 *m_present_framebuffer;
    GLuint m_texture_id;
    std::mutex m_update_mutex;
    bool new_frame;
};