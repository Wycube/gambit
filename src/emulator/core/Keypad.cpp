#include "Keypad.hpp"
#include "emulator/core/GBA.hpp"
#include "common/Bits.hpp"


namespace emu {

Keypad::Keypad(GBA &core) : m_core(core) {
    m_core.input_device.onInput([this]() { checkForInterrupt(); });
    reset();
}

void Keypad::reset() {
    m_keycnt = 0;
}

auto Keypad::read8(u32 address) -> u8 {
    switch(address) {
        case 0x130 : return bits::get<0, 8>(m_core.input_device.getKeys());
        case 0x131 : return bits::get<8, 8>(m_core.input_device.getKeys());
        case 0x132 : return bits::get<0, 8>(m_keycnt);
        case 0x133 : return bits::get<8, 8>(m_keycnt);
    }

    return 0;
}

void Keypad::write8(u32 address, u8 value) {
    m_keycnt_mutex.lock();
    switch(address) {
        case 0x132 : m_keycnt = (m_keycnt & 0xFF00) | value;
        case 0x133 : m_keycnt = (m_keycnt & 0x00FF) | ((value & 0xC3) << 8);
    }
    m_keycnt_mutex.unlock();

    checkForInterrupt();
}

//TODO: Actually end Stop mode when it is implemented
void Keypad::checkForInterrupt() {
    m_keycnt_mutex.lock();

    //Mask the currently pressed buttons with the selected buttons
    u16 pressed = ~m_core.input_device.getKeys() & (m_keycnt & 0x3FF);
    bool condition_met = false;

    if(bits::get_bit<15>(m_keycnt)) {
        //Logical AND (the masked buttons equal the selected buttons)
        condition_met = pressed == (m_keycnt & 0x3FF);
    } else {
        //Logical OR (any of the masked buttons are pressed)
        condition_met = pressed != 0;
    }

    if(condition_met) {
        //End Stop mode and request an interrupt if enabled
        //m_core.setStop(false);

        if(bits::get_bit<14>(m_keycnt)) {
            m_core.bus.requestInterrupt(INT_KEYPAD);
        }
    }

    m_keycnt_mutex.unlock();
}

} //namespace emu