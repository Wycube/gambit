#include "PPU.hpp"
#include "emulator/core/GBA.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"
#include <algorithm>


//TODO: Mosaic for non-text backgrounds

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

constexpr int OBJECT_WIDTH_LUT[16]  = {8, 16, 32, 64, 16, 32, 32, 64, 8, 8, 16, 32, 0, 0, 0, 0};
constexpr int OBJECT_HEIGHT_LUT[16] = {8, 16, 32, 64, 8, 8, 16, 32, 16, 32, 32, 64, 0, 0, 0, 0};


PPU::PPU(GBA &core) : m_core(core) {
    reset();
}

void PPU::reset() {
    m_state.dispcnt = 0x80;
    m_state.dispstat = 0;
    m_state.line = 126;

    m_state.bldcnt = 0;
    m_state.bldalpha = 0;
    m_state.bldy = 0;

    m_state.bg[0].reset();
    m_state.bg[1].reset();
    m_state.bg[2].reset();
    m_state.bg[3].reset();
    m_state.win.reset();

    std::memset(m_state.vram, 0, sizeof(m_state.vram));
    std::memset(m_state.palette, 0, sizeof(m_state.palette));
    std::memset(m_state.oam, 0, sizeof(m_state.oam));
    
    m_update_event = m_core.scheduler.generateHandle();
    m_core.scheduler.addEvent(m_update_event, [this](u64 late) { hblankStart(late); }, 960);
    LOG_DEBUG("PPU has event handle: {}", m_update_event);
}

auto PPU::readIO(u32 address) -> u8 {
    switch(address) {
        case 0x00 : return bits::get<0, 8>(m_state.dispcnt); //DISPCNT (LCD Control)
        case 0x01 : return bits::get<8, 8>(m_state.dispcnt);
        case 0x04 : return bits::get<0, 8>(m_state.dispstat); //DISPSTAT (LCD Status)
        case 0x05 : return bits::get<8, 8>(m_state.dispstat);
        case 0x06 : return m_state.line; //VCOUNT
        case 0x07 : return 0;
        case 0x08 :
        case 0x09 : return m_state.bg[0].read(address, true);
        case 0x0A :
        case 0x0B : return m_state.bg[1].read(address, true);
        case 0x0C :
        case 0x0D : return m_state.bg[2].read(address, false);
        case 0x0E :
        case 0x0F : return m_state.bg[3].read(address, false);
        case 0x48 : return bits::get<0, 8>(m_state.win.winin);
        case 0x49 : return bits::get<8, 8>(m_state.win.winin);
        case 0x4A : return bits::get<0, 8>(m_state.win.winout);
        case 0x4B : return bits::get<8, 8>(m_state.win.winout);
        case 0x50 : return bits::get<0, 8>(m_state.bldcnt);
        case 0x51 : return bits::get<8, 8>(m_state.bldcnt);
        case 0x52 : return bits::get<0, 8>(m_state.bldalpha);
        case 0x53 : return bits::get<8, 8>(m_state.bldalpha);
    }

    return 0;
}

void PPU::writeIO(u32 address, u8 value) {
    switch(address) {
        case 0x00 : m_state.dispcnt = (m_state.dispcnt & 0xFF00) | value; break;
        case 0x01 : m_state.dispcnt = (m_state.dispcnt & 0x00FF) | (value << 8); break;
        case 0x04 : m_state.dispstat = (m_state.dispstat & 0xFF07) | (value & ~7); break;
        case 0x05 : m_state.dispstat = (m_state.dispstat & 0x00FF) | (value << 8); break;
        case 0x08 :
        case 0x09 : m_state.bg[0].write(address, value); break;
        case 0x0A :
        case 0x0B : m_state.bg[1].write(address, value); break;
        case 0x0C :
        case 0x0D : m_state.bg[2].write(address, value); break;
        case 0x0E :
        case 0x0F : m_state.bg[3].write(address, value); break;
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
        case 0x2B : m_state.bg[2].reference_x = (m_state.bg[2].reference_x & ~0xFF000000) | (value << 24); m_state.bg[2].last_scanline = m_state.line; break;
        case 0x2C : m_state.bg[2].reference_y = (m_state.bg[2].reference_y & ~0x000000FF) | value; break;
        case 0x2D : m_state.bg[2].reference_y = (m_state.bg[2].reference_y & ~0x0000FF00) | (value << 8); break;
        case 0x2E : m_state.bg[2].reference_y = (m_state.bg[2].reference_y & ~0x00FF0000) | (value << 16); break;
        case 0x2F : m_state.bg[2].reference_y = (m_state.bg[2].reference_y & ~0xFF000000) | (value << 24); m_state.bg[2].last_scanline = m_state.line; break;
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
        case 0x3B : m_state.bg[3].reference_x = (m_state.bg[3].reference_x & ~0xFF000000) | (value << 24); m_state.bg[3].last_scanline = m_state.line; break;
        case 0x3C : m_state.bg[3].reference_y = (m_state.bg[3].reference_y & ~0x000000FF) | value; break;
        case 0x3D : m_state.bg[3].reference_y = (m_state.bg[3].reference_y & ~0x0000FF00) | (value << 8); break;
        case 0x3E : m_state.bg[3].reference_y = (m_state.bg[3].reference_y & ~0x00FF0000) | (value << 16); break;
        case 0x3F : m_state.bg[3].reference_y = (m_state.bg[3].reference_y & ~0xFF000000) | (value << 24); m_state.bg[3].last_scanline = m_state.line; break;
        case 0x40 : bits::set<0, 8>(m_state.win.winh[0], value); break;
        case 0x41 : bits::set<8, 8>(m_state.win.winh[0], value); break;
        case 0x42 : bits::set<0, 8>(m_state.win.winh[1], value); break;
        case 0x43 : bits::set<8, 8>(m_state.win.winh[1], value); break;
        case 0x44 : bits::set<0, 8>(m_state.win.winv[0], value); break;
        case 0x45 : bits::set<8, 8>(m_state.win.winv[0], value); break;
        case 0x46 : bits::set<0, 8>(m_state.win.winv[1], value); break;
        case 0x47 : bits::set<8, 8>(m_state.win.winv[1], value); break;
        case 0x48 : bits::set<0, 8>(m_state.win.winin, value & 0x3F); break;
        case 0x49 : bits::set<8, 8>(m_state.win.winin, value & 0x3F); break;
        case 0x4A : bits::set<0, 8>(m_state.win.winout, value & 0x3F); break;
        case 0x4B : bits::set<8, 8>(m_state.win.winout, value & 0x3F); break;
        case 0x4C : bits::set<0, 8>(m_state.mosaic, value); break;
        case 0x4D : bits::set<8, 8>(m_state.mosaic, value); break;
        case 0x4E : bits::set<16, 8>(m_state.mosaic, value); break;
        case 0x4F : bits::set<24, 8>(m_state.mosaic, value); break;
        case 0x50 : bits::set<0, 8>(m_state.bldcnt, value); break;
        case 0x51 : bits::set<8, 8>(m_state.bldcnt, value & 0x3F); break;
        case 0x52 : bits::set<0, 8>(m_state.bldalpha, value & 0x1F); break;
        case 0x53 : bits::set<8, 8>(m_state.bldalpha, value & 0x1F); break;
        case 0x54 : bits::set<0, 8>(m_state.bldy, value & 0x1F); break;
        //0x55 - 0x57 BLDY - not R/W
    }
}

template<typename T>
auto PPU::readPalette(u32 address) -> T {
    T value = 0;

    for(size_t i = 0; i < sizeof(T); i++) {
        value |= (m_state.palette[(address + i) % sizeof(m_state.palette)] << i * 8);
    }

    return value;
}

template<typename T>
auto PPU::readVRAM(u32 address) -> T {
    address %= 128_KiB;
    T value = 0;

    //VRAM mirrors the last 32k twice to make up the 128k mirror
    if(address >= 96_KiB) {
        for(size_t i = 0; i < sizeof(T); i++) {
            value |= (m_state.vram[address - 32_KiB + i] << i * 8);
        }
    } else {
        for(size_t i = 0; i < sizeof(T); i++) {
            value |= (m_state.vram[address + i] << i * 8);
        }
    }

    return value;
}

template<typename T>
auto PPU::readOAM(u32 address) -> T {
    T value = 0;

    for(size_t i = 0; i < sizeof(T); i++) {
        value |= (m_state.oam[(address + i) % sizeof(m_state.oam)] << i * 8);
    }

    return value;
}

template<typename T>
void PPU::writePalette(u32 address, T value) {
    //Byte writes affect entire addressed halfword
    if constexpr(sizeof(T) == 1) {
        m_state.palette[(address & ~1) % sizeof(m_state.palette)] = value;
        m_state.palette[(address | 1) % sizeof(m_state.palette)] = value;
    } else {
        for(size_t i = 0; i < sizeof(T); i++) {
            m_state.palette[(address + i) % sizeof(m_state.palette)] = (value >> i * 8) & 0xFF;
        }
    }
    
}

template<typename T>
void PPU::writeVRAM(u32 address, T value) {
    address %= 128_KiB;

    if constexpr(sizeof(T) == 1) {
        //TODO: What about invalid modes?
        const bool bitmap = bits::get<0, 3>(m_state.dispcnt) >= 3;
        
        if(bitmap) {
            if(address < 80_KiB || (address >= 96_KiB && address < 112_KiB)) {
                m_state.vram[address % 96_KiB]     = value;
                m_state.vram[address % 96_KiB + 1] = value;
            }
        } else {
            if(address < 64_KiB) {
                m_state.vram[address]     = value;
                m_state.vram[address + 1] = value;
            }
        }
    } else {
        if(address >= 96_KiB) {
            for(size_t i = 0; i < sizeof(T); i++) {
                m_state.vram[address - 32_KiB + i] = (value >> i * 8) & 0xFF;
            }
        } else {
            for(size_t i = 0; i < sizeof(T); i++) {
                m_state.vram[address + i] = (value >> i * 8) & 0xFF;
            }
        }
    }
}

template<typename T>
void PPU::writeOAM(u32 address, T value) {
    //Disallow byte writes
    if constexpr(sizeof(T) != 1) {
        for(size_t i = 0; i < sizeof(T); i++) {
            m_state.oam[(address + i) % sizeof(m_state.oam)] = (value >> i * 8) & 0xFF;
        }
    }
}

void PPU::hblankStart(u64 late) {
    //Draw Scanline
    if(m_state.line < 160) {
        if(!bits::get_bit<7>(m_state.dispcnt)) {
            clearBuffers();
            getWindowLine();
            drawBackground();
            drawObjects(); 
            compositeLine();
        } else {
            //Forced Blank
            static u32 blank[240] = {0};
            m_core.video_device.setLine(m_state.line, blank);
        }
    }

    //DMA Stuff
    if(m_state.line < 160) {
        m_core.dma.onHBlank();
    }

    m_core.scheduler.addEvent(m_update_event, [this](u64 late) { setHblankFlag(late); }, 46 - late);
}

void PPU::setHblankFlag(u64 late) {
    m_state.dispstat |= 2;

    //Request H-Blank interrupt if enabled
    if(bits::get_bit<4>(m_state.dispstat)) {
        m_core.bus.requestInterrupt(INT_LCD_HB);
    }

    m_core.scheduler.addEvent(m_update_event, [this](u64 late) { hblankEnd(late); }, 226 - late);
}

void PPU::hblankEnd(u64 late) {
    //VBlank flag cleared on last line
    if(m_state.line == 226) {
        m_state.dispstat &= ~1;
    }

    //End of VBlank
    if(m_state.line >= 227) {
        m_state.dispstat &= ~7;
        m_state.line = 0;
        m_state.bg[2].last_scanline = 0;
        m_state.bg[3].last_scanline = 0;
    } else {
        m_state.line++;
        m_state.dispstat &= ~2;
    }

    //Check VCOUNT
    m_state.dispstat &= ~4;
    if(m_state.line == bits::get<8, 8>(m_state.dispstat)) {
        m_state.dispstat |= 4;
        
        if(bits::get_bit<5>(m_state.dispstat)) {
            m_core.bus.requestInterrupt(INT_LCD_VC);
        }
    }

    //VBlank Start
    if(m_state.line == 160) {
        m_state.dispstat |= 1;
        m_core.video_device.presentFrame();
        m_core.dma.onVBlank();

        if(bits::get_bit<3>(m_state.dispstat)) {
            m_core.bus.requestInterrupt(INT_LCD_VB);
        }
    }

    //Video Capture DMA (scanlines 2-161)
    if(m_state.line > 1 && m_state.line < 162) {
        m_core.dma.onVideoCapture();
    } else if(m_state.line == 162) {
        m_core.dma.disableVideoCapture();
    }

    m_core.scheduler.addEvent(m_update_event, [this](u64 late) { hblankStart(late); }, 960 - late);
}

void PPU::clearBuffers() {
    std::memset(m_bmp_col, 0, sizeof(m_bmp_col));
    std::memset(m_bg_col[0], 0, sizeof(m_bg_col[0]));
    std::memset(m_bg_col[1], 0, sizeof(m_bg_col[1]));
    std::memset(m_bg_col[2], 0, sizeof(m_bg_col[2]));
    std::memset(m_bg_col[3], 0, sizeof(m_bg_col[3]));
    std::memset(m_obj_col, 0, sizeof(m_obj_col));
    std::memset(m_obj_info, 6, sizeof(m_obj_info));
}

void PPU::getWindowLine() {
    //Get objects that are windows and on this line
    std::vector<Object> win_objs;

    //TODO: Move to Window class
    if(bits::get_bit<15>(m_state.dispcnt)) {
        for(size_t i = 0; i < 128; i++) {
            u8 mode = bits::get<2, 2>(m_state.oam[i * 8 + 1]);
            u8 flags = bits::get<0, 2>(m_state.oam[i * 8 + 1]);
            
            if(mode == 2 && flags != 2) {
                const int height = OBJECT_HEIGHT_LUT[(bits::get<6, 2>(m_state.oam[i * 8 + 1]) << 2) | bits::get<6, 2>(m_state.oam[i * 8 + 3])];
                const int y = m_state.oam[i * 8];
                const int bottom = (y + (flags == 3 ? height * 2 : height)) & 0xFF;
                bool in_vertical = y <= m_state.line && m_state.line < bottom;

                if(bottom < y) {
                    in_vertical = !(y > m_state.line && m_state.line >= bottom);
                }

                //Check if object is on this line
                if(in_vertical) {
                    const int width = OBJECT_WIDTH_LUT[(bits::get<6, 2>(m_state.oam[i * 8 + 1]) << 2) | bits::get<6, 2>(m_state.oam[i * 8 + 3])];
                    const int x = ((m_state.oam[i * 8 + 3] & 1) << 8) | m_state.oam[i * 8 + 2];

                    //Check if object is visible on screen
                    if(((x + width) & 0x1FF) < x || x < 240) {
                        Object obj;
                        obj.x = x;
                        obj.y = y > bottom ? bits::sign_extend<8, int>(y) : y;
                        obj.index = i;
                        obj.width = width;
                        obj.height = height;
                        obj.affine = (flags & 1) == 1;
                        obj.double_size = flags == 3;
                        obj.param_select = bits::get<1, 5>(m_state.oam[i * 8 + 3]);
                        win_objs.push_back(std::move(obj));
                    }
                }
            }
        }
    }

    for(size_t i = 0; i < 240; i++) {
        //Bit 0-3 are bg 0-3 display, and bit 4 is obj display
        m_win_line[i] = bits::get<13, 3>(m_state.dispcnt) != 0 ? bits::get<0, 6>(m_state.win.winout) : 0x3F;

        //Window 0
        if(bits::get_bit<13>(m_state.dispcnt) && m_state.win.insideWindow(i, m_state.line, 0)) {
            m_win_line[i] = bits::get<0, 6>(m_state.win.winin);
            continue;
        }

        //Window 1
        if(bits::get_bit<14>(m_state.dispcnt) && m_state.win.insideWindow(i, m_state.line, 1)) {
            m_win_line[i] = bits::get<8, 6>(m_state.win.winin);
            continue;
        }
    }

    //Object window
    for(const auto &obj : win_objs) {
        int local_y = m_state.line - obj.y;
        u32 obj_width = obj.double_size ? obj.width * 2 : obj.width;

        for(size_t i = 0; i < obj_width; i++) {
            int screen_x = obj.getScreenX(i);

            if(screen_x >= 240) {
                continue;
            }

            u8 palette_index = obj.getObjectPixel(i, local_y, m_state);

            if(palette_index != 0 && m_win_line[screen_x] == bits::get<0, 6>(m_state.win.winout)) {
                m_win_line[screen_x] = bits::get<8, 6>(m_state.win.winout);
            }
        }
    }
}

auto PPU::getSpriteLines() -> std::vector<Object> {
    std::vector<Object> lines;

    for(size_t i = 0; i < 128; i++) {
        u8 mode = bits::get<2, 2>(m_state.oam[i * 8 + 1]);
        u8 flags = bits::get<0, 2>(m_state.oam[i * 8 + 1]);

        if(mode < 2 && flags != 2) {
            const int height = OBJECT_HEIGHT_LUT[(bits::get<6, 2>(m_state.oam[i * 8 + 1]) << 2) | bits::get<6, 2>(m_state.oam[i * 8 + 3])];
            const int y = m_state.oam[i * 8];
            const int bottom = (y + (flags == 3 ? height * 2 : height)) & 0xFF;
            bool in_vertical = y <= m_state.line && m_state.line < bottom;

            if(bottom < y) {
                in_vertical = !(y > m_state.line && m_state.line >= bottom);
            }

            //Check if object is on this line
            if(in_vertical) {
                const int width = OBJECT_WIDTH_LUT[(bits::get<6, 2>(m_state.oam[i * 8 + 1]) << 2) | bits::get<6, 2>(m_state.oam[i * 8 + 3])];
                const int x = ((m_state.oam[i * 8 + 3] & 1) << 8) | m_state.oam[i * 8 + 2];

                //Check if object is visible on screen
                if(((x + width) & 0x1FF) < x || x < 240) {
                    Object obj;
                    obj.x = x;
                    obj.y = y > bottom ? bits::sign_extend<8, int>(y) : y;
                    obj.index = i;
                    obj.width = width;
                    obj.height = height;
                    obj.affine = (flags & 1) == 1;
                    obj.double_size = flags == 3;
                    obj.param_select = bits::get<1, 5>(m_state.oam[i * 8 + 3]);
                    lines.push_back(std::move(obj));
                }
            }
        }
    }

    return lines;
}

void PPU::drawObjects() {
    if(!bits::get_bit<12>(m_state.dispcnt)) {
        return;
    }

    std::vector<Object> active_objs = getSpriteLines();

    for(const auto &obj : active_objs) {
        int local_y = m_state.line - obj.y;
        const int priority = bits::get<2, 2>(m_state.oam[obj.index * 8 + 5]);
        const bool mosaic = bits::get_bit<4>(m_state.oam[obj.index * 8 + 1]);
        u32 obj_width = obj.double_size ? obj.width * 2 : obj.width;

        if(mosaic) {
            int mosaic_h = (bits::get<12, 4>(m_state.mosaic) + 1);
            local_y = (m_state.line / mosaic_h * mosaic_h) - obj.y;
        }

        for(size_t i = 0; i < obj_width; i++) {
            int screen_x = obj.getScreenX(i);

            if(screen_x >= 240) {
                continue;
            }

            u8 palette_index = obj.getObjectPixel(i, local_y, m_state);

            if(priority < (m_obj_info[screen_x] & 7) || m_obj_col[screen_x] == 0){
                bool is_semi_transparent = false;

                if(palette_index != 0) {
                    m_obj_col[screen_x] = palette_index;
                    is_semi_transparent = bits::get<2, 2>(m_state.oam[obj.index * 8 + 1]) == 1;
                }

                m_obj_info[screen_x] = priority | (is_semi_transparent  << 3) | (mosaic << 4);
            }
        }
    }
}

void PPU::drawBackground() {
    switch(bits::get<0, 3>(m_state.dispcnt)) {
        case 0 : //BG 0-3 Text
            for(size_t i = 0; i < 240; i++) {
                if(bits::get_bit<8>(m_state.dispcnt))  m_bg_col[0][i] = m_state.bg[0].getTextPixel(i, m_state.line, m_state.vram, m_state);
                if(bits::get_bit<9>(m_state.dispcnt))  m_bg_col[1][i] = m_state.bg[1].getTextPixel(i, m_state.line, m_state.vram, m_state);
                if(bits::get_bit<10>(m_state.dispcnt)) m_bg_col[2][i] = m_state.bg[2].getTextPixel(i, m_state.line, m_state.vram, m_state);
                if(bits::get_bit<11>(m_state.dispcnt)) m_bg_col[3][i] = m_state.bg[3].getTextPixel(i, m_state.line, m_state.vram, m_state);
            }
            break;
        case 1 : //BG 0-1 Text BG 2 Affine
            for(size_t i = 0; i < 240; i++) {
                if(bits::get_bit<8>(m_state.dispcnt))  m_bg_col[0][i] = m_state.bg[0].getTextPixel(i, m_state.line, m_state.vram, m_state);
                if(bits::get_bit<9>(m_state.dispcnt))  m_bg_col[1][i] = m_state.bg[1].getTextPixel(i, m_state.line, m_state.vram, m_state);
                if(bits::get_bit<10>(m_state.dispcnt)) m_bg_col[2][i] = m_state.bg[2].getAffinePixel(i, m_state.line, m_state.vram);
            }
            break;
        case 2 : //BG 2-3 Affine
            for(size_t i = 0; i < 240; i++) {
                if(bits::get_bit<10>(m_state.dispcnt)) m_bg_col[2][i] = m_state.bg[2].getAffinePixel(i, m_state.line, m_state.vram);
                if(bits::get_bit<11>(m_state.dispcnt)) m_bg_col[3][i] = m_state.bg[3].getAffinePixel(i, m_state.line, m_state.vram);
            }
            break;
        case 3 : //BG 2 Bitmap 1x 240x160 Frame 15-bit color
            if(bits::get_bit<10>(m_state.dispcnt)) {
                for(size_t i = 0; i < 240; i++) {
                    m_bmp_col[i] = m_state.bg[2].getBitmapPixelMode3(i, m_state.line, m_state.vram);
                }
            }
            break;
        case 4 : //BG 2 Bitmap 2x 240x160 Frames Paletted
            if(bits::get_bit<10>(m_state.dispcnt)) {
                for(size_t i = 0; i < 240; i++) {
                    m_bmp_col[i] = m_state.bg[2].getBitmapPixelMode4(i, m_state.line, m_state.vram, m_state.palette, bits::get<4, 1>(m_state.dispcnt));
                }
            }
            break;
        case 5 : //BG 2 Bitmap 2x 160x128 Frames 15-bit color
            if(bits::get_bit<10>(m_state.dispcnt)) {
                for(size_t i = 0; i < 240; i++) {
                    m_bmp_col[i] = m_state.bg[2].getBitmapPixelMode5(i, m_state.line, m_state.vram, bits::get<4, 1>(m_state.dispcnt));
                }
            }
            break;
    }
}

void PPU::compositeLine() {
    const u16 zero_color = (m_state.palette[1] << 8) | m_state.palette[0];
    const bool bitmap = bits::get<0, 3>(m_state.dispcnt) >= 3;
    u8 priorities[6];
    u32 output[240];

    for(size_t i = 0; i < 240; i++) {
        for(size_t i = 0; i < 6; i++) {
            priorities[i] = i << 3;
        }

        int mosaic = (bits::get<8, 4>(m_state.mosaic) + 1);
        int i_mosaic = (m_obj_info[i] & 0x10) == 0x10 ? i / mosaic * mosaic : i;

        //BG Pixels
        priorities[0] |= m_bg_col[0][i] != 0 && bits::get_bit<0>(m_win_line[i]) ? m_state.bg[0].priority + 1 : 6;
        priorities[1] |= m_bg_col[1][i] != 0 && bits::get_bit<1>(m_win_line[i]) ? m_state.bg[1].priority + 1 : 6;
        priorities[2] |= (bitmap || m_bg_col[2][i] != 0) && bits::get_bit<2>(m_win_line[i]) ? m_state.bg[2].priority + 1 : 6;
        priorities[3] |= m_bg_col[3][i] != 0 && bits::get_bit<3>(m_win_line[i]) ? m_state.bg[3].priority + 1 : 6;

        //Object pixel
        priorities[4] |= m_obj_col[i_mosaic] != 0 && bits::get_bit<4>(m_win_line[i]) ? m_obj_info[i_mosaic] & 7 : 6;
        
        //Backdrop
        priorities[5] |= 5;

        std::sort(&priorities[0], &priorities[6], [](const u8 &a, const u8 &b) {
            return (a & 7) < (b & 7);
        });

        u16 target_1 = 0;

        //Find first target (Topmost pixel)
        switch(priorities[0] >> 3) {
            case 0 : target_1 = (m_state.palette[m_bg_col[0][i] * 2 + 1] << 8) | m_state.palette[m_bg_col[0][i] * 2]; break;
            case 1 : target_1 = (m_state.palette[m_bg_col[1][i] * 2 + 1] << 8) | m_state.palette[m_bg_col[1][i] * 2]; break;
            case 2 : target_1 = bitmap ? m_bmp_col[i] : (m_state.palette[m_bg_col[2][i] * 2 + 1] << 8) | m_state.palette[m_bg_col[2][i] * 2]; break;
            case 3 : target_1 = (m_state.palette[m_bg_col[3][i] * 2 + 1] << 8) | m_state.palette[m_bg_col[3][i] * 2]; break;
            case 4 : target_1 = (m_state.palette[0x200 + m_obj_col[i_mosaic] * 2 + 1] << 8) | m_state.palette[0x200 + m_obj_col[i_mosaic] * 2]; break;
            case 5 : target_1 = zero_color; break;
        }

        u8 red   = bits::get<0, 5>(target_1);
        u8 green = bits::get<5, 5>(target_1);
        u8 blue  = bits::get<10, 5>(target_1);

        //TODO: If semi-transparent obj is 2nd highest priority
        bool semi_transparent = (priorities[0] >> 3) == 4 && (m_obj_info[i] & 0x8) == 0x8 && bits::get_bit(m_state.bldcnt, 8 + (priorities[1] >> 3));

        //Color Effects
        if((priorities[0] & 7) < 6 && (semi_transparent || bits::get_bit(m_state.bldcnt, priorities[0] >> 3)) && bits::get_bit<5>(m_win_line[i])) {
            switch(semi_transparent ? 1 : bits::get<6, 2>(m_state.bldcnt)) {
                case 1 : //Alpha Blending
                    if(bits::get_bit(m_state.bldcnt, 8 + (priorities[1] >> 3))) {
                        float blend_a1 = (float)(m_state.bldalpha & 0x1F) / 16.0f;
                        if(blend_a1 > 1.0f) {
                            blend_a1 = 1.0f;
                        }
                        float blend_a2 = (float)((m_state.bldalpha >> 8) & 0x1F) / 16.0f;
                        if(blend_a2 > 1.0f) {
                            blend_a2 = 1.0f;
                        }
                        u16 target_2 = 0;

                        //Find second target (Next topmost pixel)
                        switch(priorities[1] >> 3) {
                            case 0 : target_2 = (m_state.palette[m_bg_col[0][i] * 2 + 1] << 8) | m_state.palette[m_bg_col[0][i] * 2]; break;
                            case 1 : target_2 = (m_state.palette[m_bg_col[1][i] *  2 + 1] << 8) | m_state.palette[m_bg_col[1][i] * 2]; break;
                            case 2 : target_2 = bitmap ? m_bmp_col[i] : (m_state.palette[m_bg_col[2][i] * 2 + 1] << 8) | m_state.palette[m_bg_col[2][i] * 2]; break;
                            case 3 : target_2 = (m_state.palette[m_bg_col[3][i] * 2 + 1] << 8) | m_state.palette[m_bg_col[3][i] * 2]; break;
                            case 4 : target_2 = (m_state.palette[0x200 + m_obj_col[i_mosaic] * 2 + 1] << 8) | m_state.palette[0x200 + m_obj_col[i_mosaic] * 2]; break;
                            case 5 : target_2 = zero_color; break;
                        } 

                        const u8 red_2   = bits::get<0, 5>(target_2);
                        const u8 green_2 = bits::get<5, 5>(target_2);
                        const u8 blue_2  = bits::get<10, 5>(target_2);

                        red   = blend_a1 * (float)red + blend_a2 * (float)red_2;
                        green = blend_a1 * (float)green + blend_a2 * (float)green_2;
                        blue  = blend_a1 * (float)blue + blend_a2 * (float)blue_2;
                    }
                    break;
                case 2 : { //Brightness Increase
                    float blend_y = (float)(m_state.bldy & 0x1F) / 16.0f;
                    if(blend_y > 1.0f) {
                        blend_y = 1.0f;
                    }

                    red   = (float)red + blend_y * (float)(31 - red);
                    green = (float)green + blend_y * (float)(31 - green);
                    blue  = (float)blue + blend_y * (float)(31 - blue);
                    break;
                }
                case 3 : { //Brightness Decrease
                    float blend_y = (float)(m_state.bldy & 0x1F) / 16.0f;
                    if(blend_y > 1.0f) {
                        blend_y = 1.0f;
                    }

                    red   = (float)red - blend_y * (float)red;
                    green = (float)green - blend_y * (float)green;
                    blue  = (float)blue - blend_y * (float)blue;
                    break;
                }
            }

            red   = red > 31 ? 31 : red;
            green = green > 31 ? 31 : green;
            blue  = blue > 31 ? 31 : blue;
        }

    
        output[i] = (red * 8 << 24) | (green * 8 << 16) | (blue * 8 << 8) | 0xFF;
    }

    m_core.video_device.setLine(m_state.line, output);
}

} //namespace emu