#include "GLFWInputDevice.hpp"
#include "emulator/core/GBA.hpp"
#include "frontend/Types.hpp"


GLFWInputDevice::GLFWInputDevice(GLFWwindow *window) {
    glfwSetKeyCallback(window, keyCallback);
    memset(m_pressed, 0, sizeof(m_pressed));
}

auto GLFWInputDevice::getKeys() -> u16 {
    std::lock_guard lock(m_key_mutex);

    u16 keys = 0;
    for(int i = 0; i < 10; i++) {
        keys |= !m_pressed[i] << i;
    }

    return keys;
}

void GLFWInputDevice::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    CallbackUserData *user_data = reinterpret_cast<CallbackUserData*>(glfwGetWindowUserPointer(window));

    if(user_data != nullptr && action != GLFW_REPEAT) {
        emu::GBA *core = reinterpret_cast<emu::GBA*>(user_data->core);
        GLFWInputDevice &device = dynamic_cast<GLFWInputDevice&>(core->getInputDevice());
        bool pressed = action == GLFW_PRESS;

        std::lock_guard lock(device.m_key_mutex);

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