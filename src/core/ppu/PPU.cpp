#include "PPU.hpp"
#include "common/Log.hpp"
#include "common/Bits.hpp"


namespace emu {

PPU::PPU(Scheduler &scheduler) : m_scheduler(scheduler) {
    m_scheduler.addEvent([this](u32 a, u32 b) { run(a, b); }, 4);
    m_state = DRAWING;
    m_dot = 0;
    m_line = 0;
}

void PPU::run(u32 current, u32 late) {
    m_dot++;

    //We're gonna pretend modes other than mode 3 don't exist
    // for(int i = 0; i < sizeof(m_framebuffer) / 4; i++) {
    //     u16 value = (m_vram[i * 2 + 1] << 8) | m_vram[i * 2];

    //     u8 red = bits::get<0, 5>(value) * 8;
    //     u8 green = bits::get<5, 5>(value) * 8;
    //     u8 blue = bits::get<10, 5>(value) * 8;

    //     m_framebuffer[i] = (red << 24) | (green << 16) | (blue << 8) | 0xFF; //Take the 16-bit, mode 3, color and turn it into a 24-bit color value
    // }

    if(m_state != VBLANK && m_line >= 160) {
        m_state = VBLANK;

        //We're gonna pretend modes other than mode 3 don't exist
        for(int i = 0; i < sizeof(m_framebuffer) / 4; i++) {
            u16 value = (m_vram[i * 2 + 1] << 8) | m_vram[i * 2];

            u8 red = bits::get<0, 5>(value) * 8;
            u8 green = bits::get<5, 5>(value) * 8;
            u8 blue = bits::get<10, 5>(value) * 8;

            m_framebuffer[i] = (red << 24) | (green << 16) | (blue << 8) | 0xFF; //Take the 16-bit, mode 3, color and turn it into a 24-bit color value
        }
    } else if(m_state == VBLANK && m_line >= 228) {
        m_state = DRAWING;
        m_line = 0;
        m_dot = 0;
    } else if(m_state == DRAWING && m_dot >= 240) {
        m_state = HBLANK;
        //We're gonna pretend modes other than mode 3 don't exist
        for(int i = 0; i < sizeof(m_framebuffer) / 4; i++) {
            u16 value = (m_vram[i * 2 + 1] << 8) | m_vram[i * 2];

            u8 red = bits::get<0, 5>(value) * 8;
            u8 green = bits::get<5, 5>(value) * 8;
            u8 blue = bits::get<10, 5>(value) * 8;

            m_framebuffer[i] = (red << 24) | (green << 16) | (blue << 8) | 0xFF; //Take the 16-bit, mode 3, color and turn it into a 24-bit color value
        }
    } else if(m_dot >= 308) {
        m_state = m_state == VBLANK ? VBLANK : DRAWING;
        m_line++;
        m_dot = 0;
    }

    LOG_DEBUG("PPU: Dot({}), Line({}), State({})", m_dot, m_line, m_state);

    m_scheduler.addEvent([this] (u32 a, u32 b) { run(a, b); }, 4 - (late % 4));
}

auto PPU::read8(u32 address) -> u8 {
    if(address <= 0x050003FF) { //Palette RAM
        return m_palette[address - 0x05000000];
    } else if(address <= 0x06017FFF) { //Video RAM
        return m_vram[address - 0x06000000];
    } else if(address <= 0x070003FF) { //OBJ Attributes
        return m_oam[address - 0x07000000];
    }

    return 0xFF;
}

void PPU::write8(u32 address, u8 value) {
    if(address <= 0x050003FF) { //Palette RAM
        m_palette[address - 0x05000000] = value;
    } else if(address <= 0x06017FFF) { //Video RAM
        m_vram[address - 0x06000000] = value;
    } else if(address <= 0x070003FF) { //OBJ Attributes
        m_oam[address - 0x07000000] = value;
    }
}

void PPU::attachDebugger(dbg::Debugger &debugger) {
    debugger.attachPPUMem(m_vram[0], m_framebuffer[0]);
}

} //namespace emu