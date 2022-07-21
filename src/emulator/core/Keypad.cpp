#include "Keypad.hpp"
#include "common/Bits.hpp"


namespace emu {

Keypad::Keypad(InputDevice &input_device) : m_input_device(input_device) {
    m_input_device.registerCallback([this] { checkForInterrupt(); });
    reset();
}

void Keypad::reset() {
    m_keyinput = 0x3FF;
    m_keycnt = 0;
}

auto Keypad::read8(u32 address) -> u8 {
    m_keyinput = m_input_device.getKeys();

    switch(address) {
        case 0x130 : return bits::get<0, 8>(m_keyinput);
        case 0x131 : return bits::get<8, 8>(m_keyinput);
        case 0x132 : return bits::get<0, 8>(m_keycnt);
        case 0x133 : return bits::get<8, 8>(m_keycnt);
    }

    return 0;
}

void Keypad::write8(u32 address, u8 value) {
    switch(address) {
        case 0x132 : m_keycnt = (m_keycnt & 0xFF00) | value;
        case 0x133 : m_keycnt = (m_keycnt & 0x00FF) | (value & 0xF0 << 8);
    }
}

// void Keypad::set_keys(u16 keys) {
//     keys |= 0xFC00;
//     m_keyinput = ~keys;
// }

// auto Keypad::get_keys() -> u16 {
//     return m_keyinput;
// }

void Keypad::checkForInterrupt() {

}

} //namespace emu