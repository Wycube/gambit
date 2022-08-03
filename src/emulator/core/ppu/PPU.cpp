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

constexpr int OBJECT_WIDTH_LUT[16]  = {8, 16, 32, 64, 16, 32, 32, 64, 8, 8, 16, 32, 0, 0, 0, 0};
constexpr int OBJECT_HEIGHT_LUT[16] = {8, 16, 32, 64, 8, 8, 16, 32, 16, 32, 32, 64, 0, 0, 0, 0};


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

void PPU::attachDebugger(dbg::Debugger &debugger) {
    debugger.attachPPU(m_state.vram);
}

void PPU::hblankStart(u64 current, u64 late) {
    m_state.dispstat |= 2;

    //Request H-Blank interrupt if enabled
    if(bits::get_bit<4>(m_state.dispstat)) {
        m_bus.requestInterrupt(INT_LCD_HB);
    }

    //Draw Scanline
    if(m_state.line < 160) {
        clearBuffers();
        getWindowLine();
        drawBackground();
        drawObjects();
        compositeLine();
    }

    m_scheduler.addEvent("Hblank End", [this](u32 a, u32 b) { hblankEnd(a, b); }, 272 - late);
}

void PPU::hblankEnd(u64 current, u64 late) {
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

void PPU::clearBuffers() {
    memset(m_bmp_col, 0, sizeof(m_bmp_col));
    memset(m_bg_col[0], 0, sizeof(m_bg_col[0]));
    memset(m_bg_col[1], 0, sizeof(m_bg_col[0]));
    memset(m_bg_col[2], 0, sizeof(m_bg_col[0]));
    memset(m_bg_col[3], 0, sizeof(m_bg_col[0]));
    memset(m_obj_col, 0, sizeof(m_obj_col));
    memset(m_obj_prios, 4, sizeof(m_obj_prios));
}

void PPU::getWindowLine() {
    //Get objects that are windows and on this line
    std::vector<int> win_objs;

    //TODO: Move to Window class
    if(bits::get_bit<15>(m_state.dispcnt)) {
        for(int i = 0; i < 128; i++) {
            if(bits::get<2, 2>(m_state.oam[i * 8 + 1]) == 2 && bits::get<0, 2>(m_state.oam[i * 8 + 1]) == 0) {
                int height = OBJECT_HEIGHT_LUT[(bits::get<6, 2>(m_state.oam[i * 8 + 1]) << 2) | bits::get<6, 2>(m_state.oam[i * 8 + 3])];
                int top = m_state.oam[i * 8];
                int bottom = (top + height) % 0xFF;
                bool in_vertical = top <= m_state.line && m_state.line < bottom;

                if(top > bottom) {
                    in_vertical = !(top > m_state.line && m_state.line >= bottom);
                }

                if(in_vertical) {
                    win_objs.push_back(i);
                }
            }
        }
    }
    
    for(int i = 0; i < 240; i++) {
        //Bit 0-3 are bg 0-3 display, and bit 4 is obj display
        m_win_line[i] = bits::get<13, 3>(m_state.dispcnt) != 0 ? bits::get<0, 5>(m_state.win.winout) : 0x1F;
        
        //Window 0
        if(bits::get_bit<13>(m_state.dispcnt) && m_state.win.insideWindow0(i, m_state.line)) {
            m_win_line[i] = bits::get<0, 5>(m_state.win.winin);
            continue;
        }

        //Window 1
        if(bits::get_bit<14>(m_state.dispcnt) && m_state.win.insideWindow1(i, m_state.line)) {
            m_win_line[i] = bits::get<8, 5>(m_state.win.winin);
            continue;
        }

        //Object window
        for(const auto &obj : win_objs) {
            int width = OBJECT_WIDTH_LUT[(bits::get<6, 2>(m_state.oam[obj * 8 + 1]) << 2) | bits::get<6, 2>(m_state.oam[obj * 8 + 3])];
            int left = ((m_state.oam[obj * 8 + 3] & 1) << 8) | m_state.oam[obj * 8 + 2];
            int right = (left + width) % 0x1FF;
            bool in_horizontal = left <= i && i < right;
            
            if(left > right) {
                in_horizontal = !(left > i && i >= right);
            }

            if(in_horizontal) {
                m_win_line[i] = bits::get<8, 5>(m_state.win.winout);
                break;
            }
        }
    }
}

struct ObjectLine {
    int obj, x, y, width;
};

void PPU::drawObjects() {
    if(!bits::get_bit<12>(m_state.dispcnt)) {
        return;
    }

    std::vector<ObjectLine> active_objs;
    const bool linear_mapping = bits::get_bit<6>(m_state.dispcnt);

    for(int i = 127; i >= 0; i--) {
        if(bits::get<2, 2>(m_state.oam[i * 8 + 1]) == 0 && bits::get<0, 2>(m_state.oam[i * 8 + 1]) == 0) {
            const int height = OBJECT_HEIGHT_LUT[(bits::get<6, 2>(m_state.oam[i * 8 + 1]) << 2) | bits::get<6, 2>(m_state.oam[i * 8 + 3])];
            const int y = m_state.oam[i * 8];
            const int bottom = (y + height) % 0x100;
            bool in_vertical = y <= m_state.line && m_state.line < bottom;

            if(bottom < y) {
                in_vertical = !(y > m_state.line && m_state.line >= bottom);
            }

            //Check if object is on this line
            if(in_vertical) {
                const int width = OBJECT_WIDTH_LUT[(bits::get<6, 2>(m_state.oam[i * 8 + 1]) << 2) | bits::get<6, 2>(m_state.oam[i * 8 + 3])];
                const int x = ((m_state.oam[i * 8 + 3] & 1) << 8) | m_state.oam[i * 8 + 2];
                
                //Check if object is visible on screen
                if((x + width) % 0x200 < x || x < 240) {
                    active_objs.push_back(ObjectLine{i, x, y > bottom ? bits::sign_extend<8, int>(y) : y, width});
                }
            }
        }
    }

    for(const auto &obj : active_objs) {
        int local_y = m_state.line - obj.y;
        const int height = OBJECT_HEIGHT_LUT[(bits::get<6, 2>(m_state.oam[obj.obj * 8 + 1]) << 2) | bits::get<6, 2>(m_state.oam[obj.obj * 8 + 3])];
        const int priority = bits::get<2, 2>(m_state.oam[obj.obj * 8 + 5]);
        const bool mirror_x = bits::get_bit<4>(m_state.oam[obj.obj * 8 + 3]);
        const bool mirror_y = bits::get_bit<5>(m_state.oam[obj.obj * 8 + 3]);
        const int tile_index = ((m_state.oam[obj.obj * 8 + 5] & 3) << 8) | m_state.oam[obj.obj * 8 + 4];
        const bool color_mode = bits::get_bit<5>(m_state.oam[obj.obj * 8 + 1]);
        const int tile_width = color_mode ? 8 : 4;
    
        if(mirror_y) local_y = height - local_y - 1;
        int tile_y = local_y / 8;
        local_y %= 8;

        for(int i = 0; i < obj.width; i++) {
            int screen_x = (obj.x + i) % 512;

            if(screen_x >= 240 || !bits::get_bit<4>(m_win_line[screen_x])) {
                continue;
            }

            int local_x = i;

            if(mirror_x) local_x = obj.width - local_x - 1;

            int tile_x = local_x / 8;
            local_x %= 8;

            int tile_address = tile_index;
            if(linear_mapping) {
                tile_address += tile_x + tile_y * (obj.width / 8);
            } else {
                tile_address += tile_x + tile_y * 32;
            }

            u8 palette_index = m_state.vram[0x10000 + tile_address * 32 + (local_x >> !color_mode) + local_y * tile_width];
            u8 palette_index_4 = palette_index;

            if(!color_mode) {
                palette_index = (palette_index >> (local_x & 1) * 4) & 0xF;
                palette_index_4 = palette_index + bits::get<4, 4>(m_state.oam[obj.obj * 8 + 5]) * 16;
            }

            if(priority <= m_obj_prios[screen_x] && palette_index != 0) {
                m_obj_col[screen_x] = palette_index_4;
                m_obj_prios[screen_x] = priority;
            }
        }
    }
}

void PPU::drawBackground() {
    switch(bits::get<0, 3>(m_state.dispcnt)) {
        case 0 : //BG 0-3 Text
            for(int i = 0; i < 240; i++) {
                m_bg_col[0][i] = m_state.bg[0].getTextPixel(i, m_state.line, m_state.vram);
                m_bg_col[1][i] = m_state.bg[1].getTextPixel(i, m_state.line, m_state.vram);
                m_bg_col[2][i] = m_state.bg[2].getTextPixel(i, m_state.line, m_state.vram);
                m_bg_col[3][i] = m_state.bg[3].getTextPixel(i, m_state.line, m_state.vram);
            }
            break;
        case 1 : //BG 0-1 Text BG 2 Affine
            m_state.bg[2].updateAffineParams();

            for(int i = 0; i < 240; i++) {
                m_bg_col[0][i] = m_state.bg[0].getTextPixel(i, m_state.line, m_state.vram);
                m_bg_col[1][i] = m_state.bg[1].getTextPixel(i, m_state.line, m_state.vram);
                m_bg_col[2][i] = m_state.bg[2].getAffinePixel(i, m_state.line, m_state.vram);
            }
            break;
        case 2 : //BG 2-3 Affine
            m_state.bg[2].updateAffineParams();
            m_state.bg[3].updateAffineParams();

            for(int i = 0; i < 240; i++) {
                m_bg_col[2][i] = m_state.bg[2].getAffinePixel(i, m_state.line, m_state.vram);
                m_bg_col[3][i] = m_state.bg[3].getAffinePixel(i, m_state.line, m_state.vram);
            }
            break;
        case 3 : //BG 2 Bitmap 1x 240x160 Frame 15-bit color
            for(int i = 0; i < 240; i++) {
                m_bmp_col[i] = m_state.bg[2].getBitmapPixelMode3(i, m_state.line, m_state.vram);
            }
            break;
        case 4 : //BG 2 Bitmap 2x 240x160 Frames Paletted
            for(int i = 0; i < 240; i++) {
                m_bmp_col[i] = m_state.bg[2].getBitmapPixelMode4(i, m_state.line, m_state.vram, m_state.palette, bits::get<4, 1>(m_state.dispcnt));
            }
            break;
        case 5 : //BG 2 Bitmap 2x 160x128 Frames 15-bit color
            for(int i = 0; i < 240; i++) {
                m_bmp_col[i] = m_state.bg[2].getBitmapPixelMode5(i, m_state.line, m_state.vram, bits::get<4, 1>(m_state.dispcnt));
            }
            break;
    }
}

void PPU::compositeLine() {
    bool enabled_bgs[4];
    u8 bg_prios[4];
    u8 sorted_bgs[4] = {0, 1, 2, 3};
    const u16 zero_color = (m_state.palette[1] << 8) | m_state.palette[0];
    const bool bitmap = bits::get<0, 3>(m_state.dispcnt) >= 3;

    for(int i = 0; i < 4; i++) {
        enabled_bgs[i] = bits::get_bit(m_state.dispcnt, 8 + i);
        bg_prios[i] = bits::get<0, 2>(m_state.bg[i].control);
    }

    std::sort(&sorted_bgs[0], &sorted_bgs[4], [&](const u8 &a, const u8 &b) {
        return bg_prios[a] < bg_prios[b];
    });

    for(int i = 0; i < 240; i++) {
        u16 final_color = bitmap ? m_bmp_col[i] : zero_color;
        u8 bg_prio = bitmap ? bg_prios[2] : 4;

        //Background
        if(!bitmap) {
            for(int j = 0; j < 4; j++) {
                const u8 bg = sorted_bgs[j];

                if(m_bg_col[bg][i] != 0 && enabled_bgs[bg] && bits::get_bit(m_win_line[i], bg)) {
                    final_color = (m_state.palette[m_bg_col[bg][i] * 2 + 1] << 8) | m_state.palette[m_bg_col[bg][i] * 2];
                    bg_prio = bg_prios[bg];
                    break;
                }
            }
        }

        //Object
        if(m_obj_col[i] != 0 && m_obj_prios[i] <= bg_prio && bits::get_bit<4>(m_win_line[i])) {
            final_color = (m_state.palette[0x200 + m_obj_col[i] * 2 + 1] << 8) | m_state.palette[0x200 + m_obj_col[i] * 2];
        }

        const u8 red = bits::get<0, 5>(final_color) * 8;
        const u8 green = bits::get<5, 5>(final_color) * 8;
        const u8 blue = bits::get<10, 5>(final_color) * 8;
        m_video_device.setPixel(i, m_state.line, (red << 24) | (green << 16) | (blue << 8) | 0xFF);
    }
}

} //namespace emu