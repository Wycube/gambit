#include "PPU.hpp"
#include "core/cpu/CPU.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

PPU::PPU(Scheduler &scheduler, Bus &bus, CPU &cpu) : m_scheduler(scheduler), m_bus(bus), m_cpu(cpu) {
    reset();
}

auto PPU::read_io(u32 address) -> u8 {
    switch(address) {
        case 0x04000000 : return bits::get<0, 8>(m_dispcnt); //DISPCNT (LCD Control)
        case 0x04000001 : return bits::get<8, 8>(m_dispcnt);
        case 0x04000004 : return bits::get<0, 8>(m_dispstat); //DISPSTAT (LCD Status)
        case 0x04000005 : return bits::get<8, 8>(m_dispstat);
        case 0x04000006 : return m_line; //VCOUNT
        case 0x04000007 : return 0;
        case 0x04000008 : return bits::get<0, 8>(m_bg0.bg0cnt); //BG0CNT (Background 0 Control)
        case 0x04000009 : return bits::get<8, 8>(m_bg0.bg0cnt);
        case 0x0400000A : return bits::get<0, 8>(m_bg1.bg1cnt); //BG1CNT (Background 1 Control)
        case 0x0400000B : return bits::get<8, 8>(m_bg1.bg1cnt);
        case 0x0400000C : return bits::get<0, 8>(m_bg2.bg2cnt); //BG2CNT (Background 2 Control)
        case 0x0400000D : return bits::get<8, 8>(m_bg2.bg2cnt);
        case 0x0400000E : return bits::get<0, 8>(m_bg3.bg3cnt); //BG3CNT (Background 3 Control)
        case 0x0400000F : return bits::get<8, 8>(m_bg3.bg3cnt);
    }

    return 0;
}

void PPU::write_io(u32 address, u8 value) {
    switch(address) {
        case 0x04000000 : m_dispcnt = (m_dispcnt & ~0xFF) | value; break;
        case 0x04000001 : m_dispcnt = (m_dispcnt & 0xFF) | (value << 8); break;
        case 0x04000004 : m_dispstat = (m_dispstat & 0xFFE2) | (value & ~0xE2); break;
        case 0x04000005 : m_dispstat = (m_dispstat & 0xFF) | (value << 8); break;
        case 0x04000008 : m_bg0.bg0cnt = (m_bg0.bg0cnt & ~0xFF) | value; break;
        case 0x04000009 : m_bg0.bg0cnt = (m_bg0.bg0cnt & 0xFF) | (value << 8); break;
        case 0x0400000A : m_bg1.bg1cnt = (m_bg1.bg1cnt & ~0xFF) | value; break;
        case 0x0400000B : m_bg1.bg1cnt = (m_bg1.bg1cnt & 0xFF) | (value << 8); break;
        case 0x0400000C : m_bg2.bg2cnt = (m_bg2.bg2cnt & ~0xFF) | value; break;
        case 0x0400000D : m_bg2.bg2cnt = (m_bg2.bg2cnt & 0xFF) | (value << 8); break;
        case 0x0400000E : m_bg3.bg3cnt = (m_bg3.bg3cnt & ~0xFF) | value; break;
        case 0x0400000F : m_bg3.bg3cnt = (m_bg3.bg3cnt & 0xFF) | (value << 8); break;
        case 0x04000010 : m_bg0.bg0hofs = (m_bg0.bg0hofs & ~0xFF) | value; printf("bg0hofs written from: %08X\n", m_cpu.getPC()); break;
        case 0x04000011 : m_bg0.bg0hofs = (m_bg0.bg0hofs & 0xFF) | ((value & 1) << 8); printf("bg0hofs written from: %08X\n", m_cpu.getPC()); break;
        case 0x04000012 : m_bg0.bg0vofs = (m_bg0.bg0vofs & ~0xFF) | value; printf("bg0vofs written from: %08X\n", m_cpu.getPC()); break;
        case 0x04000013 : m_bg0.bg0vofs = (m_bg0.bg0vofs & 0xFF) | ((value & 1) << 8); printf("bg0vofs written from: %08X\n", m_cpu.getPC()); break;
        case 0x04000014 : m_bg1.bg1hofs = (m_bg1.bg1hofs & ~0xFF) | value; break;
        case 0x04000015 : m_bg1.bg1hofs = (m_bg1.bg1hofs & 0xFF) | ((value & 1) << 8); break;
        case 0x04000016 : m_bg1.bg1vofs = (m_bg1.bg1vofs & ~0xFF) | value; break;
        case 0x04000017 : m_bg1.bg1vofs = (m_bg1.bg1vofs & 0xFF) | ((value & 1) << 8); break;
        case 0x04000018 : m_bg2.bg2hofs = (m_bg2.bg2hofs & ~0xFF) | value; break;
        case 0x04000019 : m_bg2.bg2hofs = (m_bg2.bg2hofs & 0xFF) | ((value & 1) << 8); break;
        case 0x0400001A : m_bg2.bg2vofs = (m_bg2.bg2vofs & ~0xFF) | value; break;
        case 0x0400001B : m_bg2.bg2vofs = (m_bg2.bg2vofs & 0xFF) | ((value & 1) << 8); break;
        case 0x0400001C : m_bg3.bg3hofs = (m_bg3.bg3hofs & ~0xFF) | value; break;
        case 0x0400001D : m_bg3.bg3hofs = (m_bg3.bg3hofs & 0xFF) | ((value & 1) << 8); break;
        case 0x0400001E : m_bg3.bg3vofs = (m_bg3.bg3vofs & ~0xFF) | value; break;
        case 0x0400001F : m_bg3.bg3vofs = (m_bg3.bg3vofs & 0xFF) | ((value & 1) << 8); break;
    }
}

void PPU::reset() {
    m_scheduler.addEvent("PPU Update", [this](u32 a, u32 b) { run(a, b); }, 4);
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

    LOG_DEBUG("PPU: Dot({}), Line({}), State({})", m_dot, m_line, m_state);
    m_scheduler.addEvent("PPU Update", [this] (u32 a, u32 b) { run(a, b); }, 4 - (late % 4));
}

void PPU::writeFrameMode0() {
    for(int i = 0; i < sizeof(m_framebuffer) / 4; i++) {
        m_framebuffer[i] = m_bg0.getPixelColor(i % 240, i / 240, m_vram, m_palette);
    }
}

void PPU::writeFrameMode3() {
    for(int i = 0; i < sizeof(m_framebuffer) / 4; i++) {
        u16 color = (m_vram[i * 2 + 1] << 8) | m_vram[i * 2];

        u8 red = bits::get<0, 5>(color) * 8;
        u8 green = bits::get<5, 5>(color) * 8;
        u8 blue = bits::get<10, 5>(color) * 8;

        //Take the 16-bit, mode 3, color and turn it into a 24-bit color value
        m_framebuffer[i] = (red << 24) | (green << 16) | (blue << 8) | 0xFF;
    }
}

void PPU::writeFrameMode4() {
    bool second = bits::get<4, 1>(m_dispcnt);
    u32 data_start = second ? 0xA000 : 0;

    for(int i = 0; i < sizeof(m_framebuffer) / 4; i++) {
        u8 color_index = m_vram[data_start + i];
        u16 color = (m_palette[color_index * 2 + 1] << 8) | m_palette[color_index * 2];

        u8 red = bits::get<0, 5>(color) * 8;
        u8 green = bits::get<5, 5>(color) * 8;
        u8 blue = bits::get<10, 5>(color) * 8;

        m_framebuffer[i] = (red << 24) | (green << 16) | (blue << 8) | 0xFF;
    }
}

auto PPU::read8(u32 address) -> u8 {
    if(address <= 0x04000056) {
        return read_io(address);
    } else if(address <= 0x050003FF) { //Palette RAM
        return m_palette[(address - 0x05000000) % sizeof(m_palette)];
    } else if(address <= 0x06017FFF) { //Video RAM
        return m_vram[(address - 0x06000000) % sizeof(m_vram)];
    } else if(address <= 0x070003FF) { //OBJ Attributes
        return m_oam[(address - 0x07000000) % sizeof(m_oam)];
    }

    return 0xFF;
}

void PPU::write8(u32 address, u8 value) {
    if(address <= 0x04000056) {
        write_io(address, value);
    } else if(address <= 0x050003FF) { //Palette RAM
        m_palette[(address - 0x05000000) % sizeof(m_palette)] = value;
    } else if(address <= 0x06017FFF) { //Video RAM
        m_vram[(address - 0x06000000) % sizeof(m_vram)] = value;
    } else if(address <= 0x070003FF) { //OBJ Attributes
        m_oam[(address - 0x07000000) % sizeof(m_oam)] = value;
    }
}

void PPU::attachDebugger(dbg::Debugger &debugger) {
    debugger.attachPPUMem(m_framebuffer);
}

} //namespace emu