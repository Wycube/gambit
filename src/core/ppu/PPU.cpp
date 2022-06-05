#include "PPU.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

PPU::PPU(Scheduler &scheduler) : m_scheduler(scheduler) {
    reset();
}

auto PPU::read_io(u32 address) -> u8 {
    if((address & ~1) == 0x04000000) { //DISPCNT (Display Count)
        return (m_dispcnt >> 8 * (address & 0x1)) & 0xFF;
    }
    if((address & ~1) == 0x04000004) { //DISPSTAT (Display Status)
        return (m_dispstat >> 8 * (address & 0x1)) & 0xFF;
    }

    return 0xFF;
}

void PPU::write_io(u32 address, u8 value) {
    if((address & ~1) == 0x04000000) { //DISPCNT (Display Count)
        m_dispcnt = (bits::get(8 * !(address & 0x1), 8, m_dispcnt) << 8 * !(address & 0x1)) | (value << 8 * (address & 0x1));
    }
    if((address & ~1) == 0x04000004) { //DISPSTAT (Display Status)
        m_dispstat = (bits::get(8 * !(address & 0x1), 8, m_dispstat) << 8 * !(address & 0x1)) | (value << 8 * (address & 0x1));
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

        if(mode == 3) {
            writeFrameMode3();
        } else if(mode == 4) {
            writeFrameMode4();
        }
    } else if(m_state == VBLANK && m_line >= 228) {
        m_state = DRAWING;
        m_line = 0;
        m_dot = 0;
    } else if(m_state == DRAWING && m_dot >= 240) {
        m_state = HBLANK;

        //Request H-Blank interrupt if enabled
        if(bits::get<5, 1>(m_dispstat)) {
            
        }
    } else if(m_dot >= 308) {
        if(mode == 3) {
            writeFrameMode3();
        } else if(mode == 4) {
            writeFrameMode4();
        }
        
        m_state = m_state == VBLANK ? VBLANK : DRAWING;
        m_line++;
        m_dot = 0;
    }

    LOG_DEBUG("PPU: Dot({}), Line({}), State({})", m_dot, m_line, m_state);
    m_scheduler.addEvent("PPU Update", [this] (u32 a, u32 b) { run(a, b); }, 4 - (late % 4));
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
        u16 color = (m_vram[color_index * 2 + 1] << 8) | m_vram[color_index * 2];

        u8 red = bits::get<0, 5>(color) * 8;
        u8 green = bits::get<5, 5>(color) * 8;
        u8 blue = bits::get<10, 5>(color) * 8;

        m_framebuffer[i] = (red << 24) | (green << 16) | (blue << 8) | 0xFF;
    }
}

auto PPU::read8(u32 address) -> u8 {
    if(address <= 0x04000056) {
        return read_io(address); //(m_dispcnt >> 8 * (address & 0x1)) & 0xFF;
    } else if(address <= 0x050003FF) { //Palette RAM
        return m_palette[address - 0x05000000];
    } else if(address <= 0x06017FFF) { //Video RAM
        return m_vram[address - 0x06000000];
    } else if(address <= 0x070003FF) { //OBJ Attributes
        return m_oam[address - 0x07000000];
    }

    return 0xFF;
}

void PPU::write8(u32 address, u8 value) {
    if(address <= 0x04000056) {
        write_io(address, value); //m_dispcnt = (bits::get(8 * !(address & 0x1), 8, m_dispcnt) << 8 * !(address & 0x1)) | (value << 8 * (address & 0x1));
    } else if(address <= 0x050003FF) { //Palette RAM
        m_palette[address - 0x05000000] = value;
    } else if(address <= 0x06017FFF) { //Video RAM
        m_vram[address - 0x06000000] = value;
    } else if(address <= 0x070003FF) { //OBJ Attributes
        m_oam[address - 0x07000000] = value;
    }
}

void PPU::attachDebugger(dbg::Debugger &debugger) {
    debugger.attachPPUMem(m_framebuffer);
}

} //namespace emu