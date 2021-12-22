#include "CPU.hpp"


namespace emu {

void CPU::step() {
    
}

void CPU::attachBus(Bus *bus) {
    m_bus = bus;
}

} //namespace emu