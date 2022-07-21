#include "GLFWInputDevice.hpp"
#include "emulator/core/GBA.hpp"


GLFWInputDevice::GLFWInputDevice(GLFWwindow *window) {
    glfwSetKeyCallback(window, keyCallback);
}

auto GLFWInputDevice::getKeys() -> u16 {
    u16 keys = 0;
    for(int i = 0; i < 10; i++) {
        keys |= !m_pressed[i] << i;
    }

    return keys;
}

void GLFWInputDevice::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    emu::GBA *instance = reinterpret_cast<emu::GBA*>(glfwGetWindowUserPointer(window));

    if(instance != nullptr && action != GLFW_REPEAT) {
        GLFWInputDevice &device = dynamic_cast<GLFWInputDevice&>(instance->getInputDevice());
        bool pressed = action == GLFW_PRESS;

        switch(key) {
            case GLFW_KEY_UP    : device.m_pressed[emu::KeypadInput::UP] = pressed; break;
            case GLFW_KEY_DOWN  : device.m_pressed[emu::KeypadInput::DOWN] = pressed; break;
            case GLFW_KEY_RIGHT : device.m_pressed[emu::KeypadInput::RIGHT] = pressed; break;
            case GLFW_KEY_LEFT  : device.m_pressed[emu::KeypadInput::LEFT] = pressed; break;
            case GLFW_KEY_A     : device.m_pressed[emu::KeypadInput::BUTTON_A] = pressed; break;
            case GLFW_KEY_S     : device.m_pressed[emu::KeypadInput::BUTTON_B] = pressed; break;
            case GLFW_KEY_Z     : device.m_pressed[emu::KeypadInput::START] = pressed; break;
            case GLFW_KEY_X     : device.m_pressed[emu::KeypadInput::SELECT] = pressed; break;
            case GLFW_KEY_Q     : device.m_pressed[emu::KeypadInput::BUTTON_L] = pressed; break;
            case GLFW_KEY_W     : device.m_pressed[emu::KeypadInput::BUTTON_R] = pressed; break;
        }

        device.m_callback();
    }
}