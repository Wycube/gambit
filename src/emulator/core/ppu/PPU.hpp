#pragma once

#include "Types.hpp"
#include "emulator/core/Scheduler.hpp"
#include <vector>


namespace emu {

class GBA;

class PPU final {
public:

    explicit PPU(GBA &core);

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

    void hblankStart(u64 late);
    void setHblankFlag(u64 late);
    void hblankEnd(u64 late);

    void clearBuffers();
    void getWindowLine();
    auto getSpriteLines() -> std::vector<Object>;
    void drawObjects();
    void drawBackground();
    void compositeLine();
    
    PPUState state;
    u8 win_line[240];
    u16 bmp_col[240];
    u16 bg_col[4][240];
    u16 obj_col[240];
    u8 obj_info[240];

    GBA &core;
    EventHandle hblank_start_event, hblank_flag_event, hblank_end_event;
};

} //namespace emu