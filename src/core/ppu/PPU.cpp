#include "PPU.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

PPU::PPU(Scheduler &scheduler, Bus &bus) : m_scheduler(scheduler), m_bus(bus) {
    reset();
}

auto PPU::read_io(u32 address) -> u8 {
    switch(address) {
        case 0x00 : return bits::get<0, 8>(m_dispcnt); //DISPCNT (LCD Control)
        case 0x01 : return bits::get<8, 8>(m_dispcnt);
        case 0x04 : return bits::get<0, 8>(m_dispstat); //DISPSTAT (LCD Status)
        case 0x05 : return bits::get<8, 8>(m_dispstat);
        case 0x06 : return m_line; //VCOUNT
        case 0x07 : return 0;
        case 0x08 : return bits::get<0, 8>(m_bg0.bg0cnt); //BG0CNT (Background 0 Control)
        case 0x09 : return bits::get<8, 8>(m_bg0.bg0cnt);
        case 0x0A : return bits::get<0, 8>(m_bg1.bg1cnt); //BG1CNT (Background 1 Control)
        case 0x0B : return bits::get<8, 8>(m_bg1.bg1cnt);
        case 0x0C : return bits::get<0, 8>(m_bg2.bg2cnt); //BG2CNT (Background 2 Control)
        case 0x0D : return bits::get<8, 8>(m_bg2.bg2cnt);
        case 0x0E : return bits::get<0, 8>(m_bg3.bg3cnt); //BG3CNT (Background 3 Control)
        case 0x0F : return bits::get<8, 8>(m_bg3.bg3cnt);
    }

    return 0;
}

void PPU::write_io(u32 address, u8 value) {
    switch(address) {
        case 0x00 : m_dispcnt = (m_dispcnt & ~0xFF) | value; break;
        case 0x01 : m_dispcnt = (m_dispcnt & 0xFF) | (value << 8); break;
        case 0x04 : m_dispstat = (m_dispstat & 0xFFE2) | (value & ~0xE2); break;
        case 0x05 : m_dispstat = (m_dispstat & 0xFF) | (value << 8); break;
        case 0x08 : m_bg0.bg0cnt = (m_bg0.bg0cnt & ~0xFF) | value; break;
        case 0x09 : m_bg0.bg0cnt = (m_bg0.bg0cnt & 0xFF) | (value << 8); break;
        case 0x0A : m_bg1.bg1cnt = (m_bg1.bg1cnt & ~0xFF) | value; break;
        case 0x0B : m_bg1.bg1cnt = (m_bg1.bg1cnt & 0xFF) | (value << 8); break;
        case 0x0C : m_bg2.bg2cnt = (m_bg2.bg2cnt & ~0xFF) | value; break;
        case 0x0D : m_bg2.bg2cnt = (m_bg2.bg2cnt & 0xFF) | (value << 8); break;
        case 0x0E : m_bg3.bg3cnt = (m_bg3.bg3cnt & ~0xFF) | value; break;
        case 0x0F : m_bg3.bg3cnt = (m_bg3.bg3cnt & 0xFF) | (value << 8); break;
        case 0x10 : m_bg0.bg0hofs = (m_bg0.bg0hofs & ~0xFF) | value; break;
        case 0x11 : m_bg0.bg0hofs = (m_bg0.bg0hofs & 0xFF) | ((value & 1) << 8); break;
        case 0x12 : m_bg0.bg0vofs = (m_bg0.bg0vofs & ~0xFF) | value; break;
        case 0x13 : m_bg0.bg0vofs = (m_bg0.bg0vofs & 0xFF) | ((value & 1) << 8); break;
        case 0x14 : m_bg1.bg1hofs = (m_bg1.bg1hofs & ~0xFF) | value; break;
        case 0x15 : m_bg1.bg1hofs = (m_bg1.bg1hofs & 0xFF) | ((value & 1) << 8); break;
        case 0x16 : m_bg1.bg1vofs = (m_bg1.bg1vofs & ~0xFF) | value; break;
        case 0x17 : m_bg1.bg1vofs = (m_bg1.bg1vofs & 0xFF) | ((value & 1) << 8); break;
        case 0x18 : m_bg2.bg2hofs = (m_bg2.bg2hofs & ~0xFF) | value; break;
        case 0x19 : m_bg2.bg2hofs = (m_bg2.bg2hofs & 0xFF) | ((value & 1) << 8); break;
        case 0x1A : m_bg2.bg2vofs = (m_bg2.bg2vofs & ~0xFF) | value; break;
        case 0x1B : m_bg2.bg2vofs = (m_bg2.bg2vofs & 0xFF) | ((value & 1) << 8); break;
        case 0x1C : m_bg3.bg3hofs = (m_bg3.bg3hofs & ~0xFF) | value; break;
        case 0x1D : m_bg3.bg3hofs = (m_bg3.bg3hofs & 0xFF) | ((value & 1) << 8); break;
        case 0x1E : m_bg3.bg3vofs = (m_bg3.bg3vofs & ~0xFF) | value; break;
        case 0x1F : m_bg3.bg3vofs = (m_bg3.bg3vofs & 0xFF) | ((value & 1) << 8); break;
    }
}

void PPU::hblank_start(u32 current, u32 late) {
    m_dispstat |= 2;

    //Request H-Blank interrupt if enabled
    if(bits::get<4, 1>(m_dispstat)) {
        m_bus.requestInterrupt(INT_LCD_HB);
    }

    //Draw Scanline
    if(m_line < 160) {
        u8 mode = bits::get<0, 3>(m_dispcnt);
        if(mode == 0) {
            writeFrameMode0();
        } else if(mode == 3) {
            writeFrameMode3();
        } else if(mode == 4) {
            writeFrameMode4();
        }
    }

    m_scheduler.addEvent("Hblank End", [this](u32 a, u32 b) { hblank_end(a, b); }, 272 - late);
}

void PPU::hblank_end(u32 current, u32 late) {
    m_line++;
    m_dispstat &= ~2;
    
    //Check VCOUNT
    m_dispstat &= ~4;
    if(m_line == bits::get<8, 8>(m_dispstat)) {
        m_dispstat |= 4;
        
        if(bits::get<5, 1>(m_dispstat)) {
            m_bus.requestInterrupt(INT_LCD_VC);
        }
    }

    m_scheduler.addEvent("Hblank Start", [this](u32 a, u32 b) { hblank_start(a, b); }, 960 - late);
    
    if(m_line >= 227) {
        m_dispstat &= ~3;
        m_line = 0;
        return;
    }

    if(m_line >= 160) {
        m_dispstat |= 1;

        std::memcpy(m_present_framebuffer, m_internal_framebuffer, sizeof(m_present_framebuffer));
        
        if(bits::get<3, 1>(m_dispstat)) {
            m_bus.requestInterrupt(INT_LCD_VB);
        }
    }
}

void PPU::reset() {
    m_scheduler.addEvent("Hblank Start", [this](u32 a, u32 b) { hblank_start(a, b); }, 960);
    m_state = DRAWING;
    m_dot = 0;
    m_line = 0;
}

void PPU::run(u32 current, u32 late) {
    m_dot++;
    u8 mode = bits::get<0, 3>(m_dispcnt);

    if(m_state != VBLANK && m_line >= 160) {
        m_state = VBLANK;
        m_dispstat |= 1;

        if(bits::get<3, 1>(m_dispstat)) {
            m_bus.requestInterrupt(INT_LCD_VB);
        }
    } else if(m_state == VBLANK && m_line >= 227) {
        m_state = DRAWING;
        m_line = 0;
        m_dot = 0;
        m_dispstat &= ~3;
    } else if(m_state == DRAWING && m_dot >= 240) {
        m_state = HBLANK;
        m_dispstat |= 2;

        //Request H-Blank interrupt if enabled
        if(bits::get<4, 1>(m_dispstat)) {
            m_bus.requestInterrupt(INT_LCD_HB);
        }
    } else if(m_dot >= 308) {
        if(mode == 0) {
            writeFrameMode0();
        } else if(mode == 3) {
            writeFrameMode3();
        } else if(mode == 4) {
            writeFrameMode4();
        }
        
        m_line++;
        m_dot = 0;

        m_state = m_state == VBLANK ? VBLANK : DRAWING;

        m_dispstat &= ~3;
        m_dispstat |= m_state == VBLANK;
    }

    m_dispstat &= ~4;
    if(m_line == bits::get<8, 8>(m_dispstat)) {
        m_dispstat |= 4;
        
        if(bits::get<5, 1>(m_dispstat)) {
            m_bus.requestInterrupt(INT_LCD_VC);
        }
    }

    //LOG_DEBUG("PPU: Dot({}), Line({}), State({})", m_dot, m_line, m_state);
    m_scheduler.addEvent("PPU Update", [this] (u32 a, u32 b) { run(a, b); }, 4 - (late % 4));
}

void PPU::writeFrameMode0() {
    for(int i = 0; i < 240; i++){ //sizeof(m_framebuffer) / 4; i++) {
        u32 index = i + m_line * 240;
        m_internal_framebuffer[index] = m_bg0.getPixelColor(i, m_line, m_vram, m_palette);
    }
}

void PPU::writeFrameMode3() {
    for(int i = 0; i < 240; i++) {//sizeof(m_framebuffer) / 4; i++) {
        u32 index = i + m_line * 240;
        u16 color = (m_vram[index * 2 + 1] << 8) | m_vram[index * 2];

        u8 red = bits::get<0, 5>(color) * 8;
        u8 green = bits::get<5, 5>(color) * 8;
        u8 blue = bits::get<10, 5>(color) * 8;

        //Take the 16-bit, mode 3, color and turn it into a 24-bit color value
        m_internal_framebuffer[index] = (red << 24) | (green << 16) | (blue << 8) | 0xFF;
    }
}

void PPU::writeFrameMode4() {
    bool second = bits::get<4, 1>(m_dispcnt);
    u32 data_start = second ? 0xA000 : 0;

    for(int i = 0; i < 240; i++) { //sizeof(m_framebuffer) / 4; i++) {
        u32 index = i + m_line * 240;
        u8 color_index = m_vram[data_start + index];
        u16 color = (m_palette[color_index * 2 + 1] << 8) | m_palette[color_index * 2];

        u8 red = bits::get<0, 5>(color) * 8;
        u8 green = bits::get<5, 5>(color) * 8;
        u8 blue = bits::get<10, 5>(color) * 8;

        m_internal_framebuffer[index] = (red << 24) | (green << 16) | (blue << 8) | 0xFF;
    }
}

auto PPU::readIO(u32 address) -> u8 {
    return read_io(address);
}

auto PPU::readPalette(u32 address) -> u8 {
    return m_palette[address % sizeof(m_palette)];
}

auto PPU::readVRAM(u32 address) -> u8 {
    return m_palette[address % sizeof(m_palette)];
}

auto PPU::readOAM(u32 address) -> u8 {
    return m_oam[address % sizeof(m_oam)];
}

void PPU::writeIO(u32 address, u8 value) {
    write_io(address, value);
}

void PPU::writePalette(u32 address, u8 value) {
    m_palette[address % sizeof(m_palette)] = value;
}

void PPU::writeVRAM(u32 address, u8 value) {
    m_vram[address % sizeof(m_vram)] = value;
}

void PPU::writeOAM(u32 address, u8 value) {
    m_oam[address % sizeof(m_oam)] = value;
}

void PPU::attachDebugger(dbg::Debugger &debugger) {
    debugger.attachPPUMem(m_present_framebuffer);
}

} //namespace emu