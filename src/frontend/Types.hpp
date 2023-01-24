#pragma once

#include "emulator/core/GBA.hpp"


struct CallbackUserData {
    void *frontend;
    std::shared_ptr<emu::GBA> core;
};