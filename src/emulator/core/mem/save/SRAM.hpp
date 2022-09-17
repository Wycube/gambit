#pragma once

#include "Save.hpp"


namespace emu {

class SRAM final : public Save {
public:

    SRAM();
    ~SRAM() = default;

    void reset() override;
    auto read(u32 address) -> u8 override;
    void write(u32 address, u8 value) override;
};

} //namespace emu