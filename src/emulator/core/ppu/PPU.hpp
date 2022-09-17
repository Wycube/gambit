#pragma once

#include "Types.hpp"


namespace emu {

class GBA;

class PPU final {
public:

    PPU(GBA &core);

    void reset();

    auto readIO(u32 address) -> u8;
    void writeIO(u32 address, u8 value);

    template<typename T>
    auto readPalette(u32 address) -> T;
    template<typename T>
    auto readVRAM(u32 address) -> T;
    template<typename T>
    auto readOAM(u32 address) -> T;
    template<typename T>
    void writePalette(u32 address, T value);
    template<typename T>
    void writeVRAM(u32 address, T value);
    template<typename T>
    void writeOAM(u32 address, T value);

private:

    PPUState m_state;
    u8 m_win_line[240];
    u16 m_bmp_col[240];
    u8 m_bg_col[4][240];
    u8 m_obj_col[240];
    u8 m_obj_prios[240];

    GBA &m_core;

    void hblankStart(u64 current, u64 late);
    void hblankEnd(u64 current, u64 late);

    void clearBuffers();
    void getWindowLine();
    void drawObjects();
    void drawBackground();
    void compositeLine();
};

} //namespace emu