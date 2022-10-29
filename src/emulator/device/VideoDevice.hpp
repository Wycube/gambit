#pragma once

#include "common/Types.hpp"


namespace emu {

class VideoDevice {
public:

    virtual void clear(u32 color) = 0;
    virtual void setPixel(int x, int y, u32 color) = 0;
    virtual void setLine(int y, u32 *colors) = 0;
    virtual void presentFrame() = 0;

protected:

    u32 m_internal_framebuffer[240 * 160];
};

class NullVideoDevice final : public VideoDevice {
public:

    void clear(u32 color) override { }
    void setPixel(int x, int y, u32 color) override { }
    void setLine(int y, u32 *colors) override { }
    void presentFrame() override { }
};

} //namespace emu