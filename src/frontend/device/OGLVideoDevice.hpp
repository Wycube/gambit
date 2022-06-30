#pragma once

#include "emulator/device/VideoDevice.hpp"
#include <glad/gl.h>


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

    GLuint m_texture_id;
};