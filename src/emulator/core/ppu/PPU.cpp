#include "PPU.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

template auto PPU::readPalette<u8>(u32 address) -> u8;
template auto PPU::readPalette<u16>(u32 address) -> u16;
template auto PPU::readPalette<u32>(u32 address) -> u32;
template auto PPU::readVRAM<u8>(u32 address) -> u8;
template auto PPU::readVRAM<u16>(u32 address) -> u16;
template auto PPU::readVRAM<u32>(u32 address) -> u32;
template auto PPU::readOAM<u8>(u32 address) -> u8;
template auto PPU::readOAM<u16>(u32 address) -> u16;
template auto PPU::readOAM<u32>(u32 address) -> u32;
template void PPU::writePalette<u8>(u32 address, u8 value);
template void PPU::writePalette<u16>(u32 address, u16 value);
template void PPU::writePalette<u32>(u32 address, u32 value);
template void PPU::writeVRAM<u8>(u32 address, u8 value);
template void PPU::writeVRAM<u16>(u32 address,u16 value);
template void PPU::writeVRAM<u32>(u32 address, u32 value);
template void PPU::writeOAM<u8>(u32 address, u8 value);
template void PPU::writeOAM<u16>(u32 address,u16 value);
template void PPU::writeOAM<u32>(u32 address, u32 value);


PPU::PPU(VideoDevice &video_device, Scheduler &scheduler, Bus &bus, DMA &dma) : m_video_device(video_device), m_scheduler(scheduler), m_bus(bus), m_dma(dma) {
    reset();
}

void PPU::reset() {
    m_scheduler.addEvent("Hblank Start", [this](u32 a, u32 b) { hblankStart(a, b); }, 960);
    m_state.dispcnt = 0x80;
    m_state.dispstat = 0;
    m_state.line = 125;
}

auto PPU::readIO(u32 address) -> u8 {
    switch(address) {
        case 0x00 : return bits::get<0, 8>(m_state.dispcnt); //DISPCNT (LCD Control)
        case 0x01 : return bits::get<8, 8>(m_state.dispcnt);
        case 0x04 : return bits::get<0, 8>(m_state.dispstat); //DISPSTAT (LCD Status)
        case 0x05 : return bits::get<8, 8>(m_state.dispstat);
        case 0x06 : return m_state.line; //VCOUNT
        case 0x07 : return 0;
        case 0x08 : return bits::get<0, 8>(m_state.bg[0].control); //BG0CNT (Background 0 Control)
        case 0x09 : return bits::get<8, 8>(m_state.bg[0].control);
        case 0x0A : return bits::get<0, 8>(m_state.bg[1].control); //BG1CNT (Background 1 Control)
        case 0x0B : return bits::get<8, 8>(m_state.bg[1].control);
        case 0x0C : return bits::get<0, 8>(m_state.bg[2].control); //BG2CNT (Background 2 Control)
        case 0x0D : return bits::get<8, 8>(m_state.bg[2].control);
        case 0x0E : return bits::get<0, 8>(m_state.bg[3].control); //BG3CNT (Background 3 Control)
        case 0x0F : return bits::get<8, 8>(m_state.bg[3].control);
        case 0x48 : return bits::get<0, 8>(m_state.win.winin);
        case 0x49 : return bits::get<8, 8>(m_state.win.winin);
        case 0x4A : return bits::get<0, 8>(m_state.win.winout);
        case 0x4B : return bits::get<8, 8>(m_state.win.winout);
    }

    return 0;
}

void PPU::writeIO(u32 address, u8 value) {
    switch(address) {
        case 0x00 : m_state.dispcnt = (m_state.dispcnt & ~0xFF) | value; break;
        case 0x01 : m_state.dispcnt = (m_state.dispcnt & 0xFF) | (value << 8); break;
        case 0x04 : m_state.dispstat = (m_state.dispstat & 0xFFE2) | (value & ~0xE2); break;
        case 0x05 : m_state.dispstat = (m_state.dispstat & 0xFF) | (value << 8); break;
        case 0x08 : m_state.bg[0].control = (m_state.bg[0].control & ~0xFF) | value; break;
        case 0x09 : m_state.bg[0].control = (m_state.bg[0].control & 0xFF) | (value << 8); break;
        case 0x0A : m_state.bg[1].control = (m_state.bg[1].control & ~0xFF) | value; break;
        case 0x0B : m_state.bg[1].control = (m_state.bg[1].control & 0xFF) | (value << 8); break;
        case 0x0C : m_state.bg[2].control = (m_state.bg[2].control & ~0xFF) | value; break;
        case 0x0D : m_state.bg[2].control = (m_state.bg[2].control & 0xFF) | (value << 8); break;
        case 0x0E : m_state.bg[3].control = (m_state.bg[3].control & ~0xFF) | value; break;
        case 0x0F : m_state.bg[3].control = (m_state.bg[3].control & 0xFF) | (value << 8); break;
        case 0x10 : m_state.bg[0].h_offset = (m_state.bg[0].h_offset & ~0xFF) | value; break;
        case 0x11 : m_state.bg[0].h_offset = (m_state.bg[0].h_offset & 0xFF) | ((value & 1) << 8); break;
        case 0x12 : m_state.bg[0].v_offset = (m_state.bg[0].v_offset & ~0xFF) | value; break;
        case 0x13 : m_state.bg[0].v_offset = (m_state.bg[0].v_offset & 0xFF) | ((value & 1) << 8); break;
        case 0x14 : m_state.bg[1].h_offset = (m_state.bg[1].h_offset & ~0xFF) | value; break;
        case 0x15 : m_state.bg[1].h_offset = (m_state.bg[1].h_offset & 0xFF) | ((value & 1) << 8); break;
        case 0x16 : m_state.bg[1].v_offset = (m_state.bg[1].v_offset & ~0xFF) | value; break;
        case 0x17 : m_state.bg[1].v_offset = (m_state.bg[1].v_offset & 0xFF) | ((value & 1) << 8); break;
        case 0x18 : m_state.bg[2].h_offset = (m_state.bg[2].h_offset & ~0xFF) | value; break;
        case 0x19 : m_state.bg[2].h_offset = (m_state.bg[2].h_offset & 0xFF) | ((value & 1) << 8); break;
        case 0x1A : m_state.bg[2].v_offset = (m_state.bg[2].v_offset & ~0xFF) | value; break;
        case 0x1B : m_state.bg[2].v_offset = (m_state.bg[2].v_offset & 0xFF) | ((value & 1) << 8); break;
        case 0x1C : m_state.bg[3].h_offset = (m_state.bg[3].h_offset & ~0xFF) | value; break;
        case 0x1D : m_state.bg[3].h_offset = (m_state.bg[3].h_offset & 0xFF) | ((value & 1) << 8); break;
        case 0x1E : m_state.bg[3].v_offset = (m_state.bg[3].v_offset & ~0xFF) | value; break;
        case 0x1F : m_state.bg[3].v_offset = (m_state.bg[3].v_offset & 0xFF) | ((value & 1) << 8); break;
        case 0x20 : m_state.bg[2].param_a = (m_state.bg[2].param_a & ~0xFF) | value; break;
        case 0x21 : m_state.bg[2].param_a = (m_state.bg[2].param_a & 0xFF) | (value << 8); break;
        case 0x22 : m_state.bg[2].param_b = (m_state.bg[2].param_b & ~0xFF) | value; break;
        case 0x23 : m_state.bg[2].param_b = (m_state.bg[2].param_b & 0xFF) | (value << 8); break;
        case 0x24 : m_state.bg[2].param_c = (m_state.bg[2].param_c & ~0xFF) | value; break;
        case 0x25 : m_state.bg[2].param_c = (m_state.bg[2].param_c & 0xFF) | (value << 8); break;
        case 0x26 : m_state.bg[2].param_d = (m_state.bg[2].param_d & ~0xFF) | value; break;
        case 0x27 : m_state.bg[2].param_d = (m_state.bg[2].param_d & 0xFF) | (value << 8); break;
        case 0x28 : m_state.bg[2].reference_x = (m_state.bg[2].reference_x & ~0x000000FF) | value; break;
        case 0x29 : m_state.bg[2].reference_x = (m_state.bg[2].reference_x & ~0x0000FF00) | (value << 8); break;
        case 0x2A : m_state.bg[2].reference_x = (m_state.bg[2].reference_x & ~0x00FF0000) | (value << 16); break;
        case 0x2B : m_state.bg[2].reference_x = (m_state.bg[2].reference_x & ~0xFF000000) | (value << 24); break;
        case 0x2C : m_state.bg[2].reference_y = (m_state.bg[2].reference_y & ~0x000000FF) | value; break;
        case 0x2D : m_state.bg[2].reference_y = (m_state.bg[2].reference_y & ~0x0000FF00) | (value << 8); break;
        case 0x2E : m_state.bg[2].reference_y = (m_state.bg[2].reference_y & ~0x00FF0000) | (value << 16); break;
        case 0x2F : m_state.bg[2].reference_y = (m_state.bg[2].reference_y & ~0xFF000000) | (value << 24); break;
        case 0x30 : m_state.bg[3].param_a = (m_state.bg[3].param_a & ~0xFF) | value; break;
        case 0x31 : m_state.bg[3].param_a = (m_state.bg[3].param_a & 0xFF) | (value << 8); break;
        case 0x32 : m_state.bg[3].param_b = (m_state.bg[3].param_b & ~0xFF) | value; break;
        case 0x33 : m_state.bg[3].param_b = (m_state.bg[3].param_b & 0xFF) | (value << 8); break;
        case 0x34 : m_state.bg[3].param_c = (m_state.bg[3].param_c & ~0xFF) | value; break;
        case 0x35 : m_state.bg[3].param_c = (m_state.bg[3].param_c & 0xFF) | (value << 8); break;
        case 0x36 : m_state.bg[3].param_d = (m_state.bg[3].param_d & ~0xFF) | value; break;
        case 0x37 : m_state.bg[3].param_d = (m_state.bg[3].param_d & 0xFF) | (value << 8); break;
        case 0x38 : m_state.bg[3].reference_x = (m_state.bg[3].reference_x & ~0x000000FF) | value; break;
        case 0x39 : m_state.bg[3].reference_x = (m_state.bg[3].reference_x & ~0x0000FF00) | (value << 8); break;
        case 0x3A : m_state.bg[3].reference_x = (m_state.bg[3].reference_x & ~0x00FF0000) | (value << 16); break;
        case 0x3B : m_state.bg[3].reference_x = (m_state.bg[3].reference_x & ~0xFF000000) | (value << 24); break;
        case 0x3C : m_state.bg[3].reference_y = (m_state.bg[3].reference_y & ~0x000000FF) | value; break;
        case 0x3D : m_state.bg[3].reference_y = (m_state.bg[3].reference_y & ~0x0000FF00) | (value << 8); break;
        case 0x3E : m_state.bg[3].reference_y = (m_state.bg[3].reference_y & ~0x00FF0000) | (value << 16); break;
        case 0x3F : m_state.bg[3].reference_y = (m_state.bg[3].reference_y & ~0xFF000000) | (value << 24); break;
        case 0x40 : bits::set<0, 8>(m_state.win.win0h, value); break;
        case 0x41 : bits::set<8, 8>(m_state.win.win0h, value); break;
        case 0x42 : bits::set<0, 8>(m_state.win.win1h, value); break;
        case 0x43 : bits::set<8, 8>(m_state.win.win1h, value); break;
        case 0x44 : bits::set<0, 8>(m_state.win.win0v, value); break;
        case 0x45 : bits::set<8, 8>(m_state.win.win0v, value); break;
        case 0x46 : bits::set<0, 8>(m_state.win.win1v, value); break;
        case 0x47 : bits::set<8, 8>(m_state.win.win1v, value); break;
        case 0x48 : bits::set<0, 8>(m_state.win.winin, value); break;
        case 0x49 : bits::set<8, 8>(m_state.win.winin, value); break;
        case 0x4A : bits::set<0, 8>(m_state.win.winout, value); break;
        case 0x4B : bits::set<8, 8>(m_state.win.winout, value); break;
    }
}

template<typename T>
auto PPU::readPalette(u32 address) -> T {
    T value = 0;
    
    for(int i = 0; i < sizeof(T); i++) {
        value |= (m_state.palette[(address + i) % sizeof(m_state.palette)] << i * 8);
    }

    return value;
}

template<typename T>
auto PPU::readVRAM(u32 address) -> T {
    address %= 128_KiB;
    T value = 0;

    if(address >= 96_KiB) {
        for(int i = 0; i < sizeof(T); i++) {
            value |= (m_state.vram[address - 32_KiB + i] << i * 8);
        }
    } else {
        for(int i = 0; i < sizeof(T); i++) {
            value |= (m_state.vram[address + i] << i * 8);
        }
    }

    return value;
}

template<typename T>
auto PPU::readOAM(u32 address) -> T {
    T value = 0;
    
    for(int i = 0; i < sizeof(T); i++) {
        value |= (m_state.oam[(address + i) % sizeof(m_state.oam)] << i * 8);
    }

    return value;
}

template<typename T>
void PPU::writePalette(u32 address, T value) {
    for(int i = 0; i < sizeof(T); i++) {
        m_state.palette[(address + i) % sizeof(m_state.palette)] = (value >> i * 8) & 0xFF;
    }
}

template<typename T>
void PPU::writeVRAM(u32 address, T value) {
    if constexpr(sizeof(T) == 1) {
        return;
    }

    address %= 128_KiB;

    if(address >= 96_KiB) {
        for(int i = 0; i < sizeof(T); i++) {
            m_state.vram[address - 32_KiB + i] = (value >> i * 8) & 0xFF;
        }
    } else {
        for(int i = 0; i < sizeof(T); i++) {
            m_state.vram[address + i] = (value >> i * 8) & 0xFF;
        }
    }
}

template<typename T>
void PPU::writeOAM(u32 address, T value) {
    for(int i = 0; i < sizeof(T); i++) {
        m_state.oam[(address + i) % sizeof(m_state.oam)] = (value >> i * 8) & 0xFF;
    }
}

void PPU::hblankStart(u32 current, u32 late) {
    m_state.dispstat |= 2;

    //Request H-Blank interrupt if enabled
    if(bits::get_bit<4>(m_state.dispstat)) {
        m_bus.requestInterrupt(INT_LCD_HB);
    }

    //Draw Scanline
    if(m_state.line < 160) {
        u8 mode = bits::get<0, 3>(m_state.dispcnt);
        if(mode == 0) {
            writeLineMode0();
        } else if(mode == 1) {
            writeLineMode1();
        } else if(mode == 3) {
            writeLineMode3();
        } else if(mode == 4) {
            writeLineMode4();
        } else if(mode == 5) {
            writeLineMode5();
        }
        writeObjects();
    }

    m_scheduler.addEvent("Hblank End", [this](u32 a, u32 b) { hblankEnd(a, b); }, 272 - late);
}

void PPU::hblankEnd(u32 current, u32 late) {
    m_state.line++;
    m_state.dispstat &= ~2;
    
    //Check VCOUNT
    m_state.dispstat &= ~4;
    if(m_state.line == bits::get<8, 8>(m_state.dispstat)) {
        m_state.dispstat |= 4;
        
        if(bits::get_bit<5>(m_state.dispstat)) {
            m_bus.requestInterrupt(INT_LCD_VC);
        }
    }

    //DMA Stuff
    if(m_state.line < 160) {
        m_dma.onHBlank();
    }

    m_scheduler.addEvent("Hblank Start", [this](u32 a, u32 b) { hblankStart(a, b); }, 960 - late);
    
    if(m_state.line == 226) {
        m_state.dispstat &= ~1;
    }

    if(m_state.line >= 227) {
        m_state.dispstat &= ~7;
        m_state.line = 0;
        return;
    }

    if(m_state.line == 160) {
        m_state.dispstat |= 1;
        m_video_device.presentFrame();
        m_dma.onVBlank();

        if(bits::get_bit<3>(m_state.dispstat)) {
            LOG_DEBUG("VBLANK interrupt requested");
            m_bus.requestInterrupt(INT_LCD_VB);
        }
    }
}

void PPU::writeObjects() {
    std::vector<u32> active_objs;
    static const int WIDTH_LUT[16]  = {8, 16, 32, 64, 16, 32, 32, 64, 8, 8, 16, 32, 0, 0, 0, 0};
    static const int HEIGHT_LUT[16] = {8, 16, 32, 64, 8, 8, 16, 32, 16, 32, 32, 64, 0, 0, 0, 0};

    for(int i = 0; i < 128; i++) {
        if(bits::get<2, 2>(m_state.oam[i * 8 + 1]) == 0 && bits::get<0, 2>(m_state.oam[i * 8 + 1]) == 0) {
            int y = m_state.oam[i * 8];
            int height = HEIGHT_LUT[(bits::get<6, 2>(m_state.oam[i * 8 + 1]) << 2) | bits::get<6, 2>(m_state.oam[i * 8 + 3])];
            if(y <= m_state.line && y + height > m_state.line) {
                active_objs.push_back(i);
            }
        }
    }

    for(int i = 0; i < 240; i++) {
        for(u32 obj : active_objs) {
            int x = (m_state.oam[obj * 8 + 3] & 1 << 8) | m_state.oam[obj * 8 + 2];
            int y = m_state.oam[obj * 8];
            int width = WIDTH_LUT[(bits::get<6, 2>(m_state.oam[obj * 8 + 1]) << 2) | bits::get<6, 2>(m_state.oam[obj * 8 + 3])];
            int height = HEIGHT_LUT[(bits::get<6, 2>(m_state.oam[obj * 8 + 1]) << 2) | bits::get<6, 2>(m_state.oam[obj * 8 + 3])];
            if(x <= i && x + width > i) {
                bool mirror_x = bits::get_bit<4>(m_state.oam[obj * 8 + 3]);
                bool mirror_y = bits::get_bit<5>(m_state.oam[obj * 8 + 3]);
                int local_x = i - x;
                int local_y = m_state.line - y;

                if(mirror_x) local_x = width - local_x;
                if(mirror_y) local_y = height - local_y;

                int tile_x = local_x / 8;
                int tile_y = local_y / 8;
                local_x %= 8;
                local_y %= 8;
                int tile_index = (m_state.oam[obj * 8 + 5] & 3 << 8) | m_state.oam[obj * 8 + 4];
                tile_index += tile_x + tile_y * (width / 8);
                bool color_mode = bits::get_bit<5>(m_state.oam[obj * 8 + 1]);
                u8 tile_width = color_mode ? 8 : 4;

                u8 palette_index = m_state.vram[0x10000 + tile_index * 32 + (local_x >> !color_mode) + local_y * tile_width];

                if(!color_mode) {
                    palette_index = (palette_index >> (local_x & 1) * 4) & 0xF;

                    if(palette_index == 0) {
                        continue;
                    }

                    palette_index += bits::get<4, 4>(m_state.oam[obj * 8 + 5]) * 16;
                }

                if(palette_index == 0) {
                    continue;
                }

                u16 color = (m_state.palette[0x200 + palette_index * 2 + 1] << 8) | m_state.palette[0x200 + palette_index * 2];
                u8 red = bits::get<0, 5>(color) * 8;
                u8 green = bits::get<5, 5>(color) * 8;
                u8 blue = bits::get<10, 5>(color) * 8;

                m_video_device.setPixel(i, m_state.line, (red << 24) | (green << 16) | (blue << 8) | 0xFF);
            }
        }
    }
}

void PPU::writeLineMode0() {
    bool enabled_bgs[4] = {bits::get_bit<8>(m_state.dispcnt), bits::get_bit<9>(m_state.dispcnt), bits::get_bit<10>(m_state.dispcnt), bits::get_bit<11>(m_state.dispcnt)};
    u8 sorted_bgs[4] = {0, 1, 2, 3};

    std::stable_sort(&sorted_bgs[0], &sorted_bgs[3], [this](const u8 &a, const u8 &b) {
        return bits::get<0, 2>(m_state.bg[a].control) < bits::get<0, 2>(m_state.bg[b].control);
    });
    
    for(int i = 0; i < 240; i++) {
        u8 palette_index = 0;
        
        for(int j = 0; j < 4; j++) {
            if(!enabled_bgs[j] || !m_state.win.isPixelDisplayed(i, m_state.line, j, m_state.dispcnt)) {
                continue;
            }

            palette_index = m_state.bg[sorted_bgs[j]].getTextPixel(i, m_state.line, m_state.vram, m_state.palette);

            //0 is the transparent index
            if(palette_index != 0) {
                break;
            }
        }

        u16 color_16 = (m_state.palette[palette_index * 2 + 1] << 8) | m_state.palette[palette_index * 2];
        u8 red = bits::get<0, 5>(color_16) * 8;
        u8 green = bits::get<5, 5>(color_16) * 8;
        u8 blue = bits::get<10, 5>(color_16) * 8;

        m_video_device.setPixel(i, m_state.line, (red << 24) | (green << 16) | (blue << 8) | 0xFF);
    }
}

void PPU::writeLineMode1() {
    m_state.bg[2].updateAffineParams();

    u8 palette_index;

    for(int i = 0; i < 240; i++) {
        // u8 palette_index = m_bg[2].getAffinePixel(i, m_line, m_vram, m_palette);
        palette_index = m_state.bg[0].getTextPixel(i, m_state.line, m_state.vram, m_state.palette);

        if(palette_index == 0) {
            palette_index = m_state.bg[2].getAffinePixel(i, m_state.line, m_state.vram, m_state.palette);
        }

        u16 color_16 = (m_state.palette[palette_index * 2 + 1] << 8) | m_state.palette[palette_index * 2];
        u8 red = bits::get<0, 5>(color_16) * 8;
        u8 green = bits::get<5, 5>(color_16) * 8;
        u8 blue = bits::get<10, 5>(color_16) * 8;

        m_video_device.setPixel(i, m_state.line, (red << 24) | (green << 16) | (blue << 8) | 0xFF);
    }
}

void PPU::writeLineMode3() {
    for(int i = 0; i < 240; i++) {
        m_video_device.setPixel(i, m_state.line, m_state.bg[2].getBitmapPixelMode3(i, m_state.line, m_state.vram));
    }
}

void PPU::writeLineMode4() {
    bool frame_1 = bits::get<4, 1>(m_state.dispcnt);

    for(int i = 0; i < 240; i++) {
        m_video_device.setPixel(i, m_state.line, m_state.bg[2].getBitmapPixelMode4(i, m_state.line, m_state.vram, m_state.palette, frame_1));
    }
}

void PPU::writeLineMode5() {
    bool frame_1 = bits::get<4, 1>(m_state.dispcnt);

    for(int i = 0; i < 240; i++) {
        m_video_device.setPixel(i, m_state.line, m_state.bg[2].getBitmapPixelMode5(i, m_state.line, m_state.vram, frame_1));
    }
}

void PPU::attachDebugger(dbg::Debugger &debugger) {
    debugger.attachPPU(m_state.vram);
}

} //namespace emu