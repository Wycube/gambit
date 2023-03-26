#pragma once

#include "common/Types.hpp"


namespace emu {

class VideoDevice {
public:

    virtual void setPixel(int x, int y, u32 color) = 0;
    virtual void setLine(int y, const u32 *colors) = 0;
    virtual void presentFrame() = 0;
};

} //namespace emu