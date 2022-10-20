#include "GLFWInputDevice.hpp"
#include "emulator/core/GBA.hpp"
#include "frontend/Types.hpp"
#include "common/Log.hpp"
#include <cstring>


GLFWInputDevice::GLFWInputDevice(GLFWwindow *window) {
    glfwSetKeyCallback(window, keyCallback);
    m_joystick_connected = false;
    memset(m_pressed, 0, sizeof(m_pressed));
}

void GLFWInputDevice::update() {
    if(m_joystick_connected) {
        //Check if still connected
        m_joystick_connected = glfwJoystickPresent(m_current_joystick) == GLFW_TRUE;

        if(!m_joystick_connected) {
            LOG_INFO("Controller in slot {} disconnected: {}", m_current_joystick, glfwGetJoystickName(m_current_joystick));

            return;
        }

        //Get input from joystick
        GLFWgamepadstate state;
        glfwGetGamepadState(m_current_joystick, &state);

        std::lock_guard lock(m_key_mutex); 

        m_pressed[emu::KeypadInput::UP]       = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_PRESS;
        m_pressed[emu::KeypadInput::DOWN]     = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_PRESS;
        m_pressed[emu::KeypadInput::LEFT]     = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] == GLFW_PRESS;
        m_pressed[emu::KeypadInput::RIGHT]    = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] == GLFW_PRESS;
        m_pressed[emu::KeypadInput::BUTTON_A] = state.buttons[GLFW_GAMEPAD_BUTTON_B] == GLFW_PRESS;
        m_pressed[emu::KeypadInput::BUTTON_B] = state.buttons[GLFW_GAMEPAD_BUTTON_A] == GLFW_PRESS;
        // m_pressed[emu::KeypadInput::BUTTON_L] = state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] == GLFW_PRESS;
        // m_pressed[emu::KeypadInput::BUTTON_R] = state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] == GLFW_PRESS;
        m_pressed[emu::KeypadInput::SELECT]   = state.buttons[GLFW_GAMEPAD_BUTTON_GUIDE] == GLFW_PRESS;
        m_pressed[emu::KeypadInput::START]    = state.buttons[GLFW_GAMEPAD_BUTTON_START] == GLFW_PRESS;

        //TODO: Add configurable deadzones
        m_pressed[emu::KeypadInput::UP]       = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] < -0.3f;
        m_pressed[emu::KeypadInput::DOWN]     = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] > 0.3f;
        m_pressed[emu::KeypadInput::LEFT]     = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] < -0.3f;
        m_pressed[emu::KeypadInput::RIGHT]    = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] > 0.3f;
        m_pressed[emu::KeypadInput::BUTTON_L] = state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > 0.3f;
        m_pressed[emu::KeypadInput::BUTTON_R] = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.3f;

        //TODO: Call input callback if necessary
    } else {
        //Check for connections
        for(int i = 0; i < GLFW_JOYSTICK_LAST; i++) {
            if(glfwJoystickPresent(i) == GLFW_TRUE && glfwJoystickIsGamepad(i)) {
                LOG_INFO("New controller connection in slot {}: {} {}", i, glfwGetJoystickName(i), glfwGetGamepadName(i));
                m_current_joystick = i;
                m_joystick_connected = true;
            }
        }
    }
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
        GLFWInputDevice &device = dynamic_cast<GLFWInputDevice&>(core->input_device);

        if(device.m_joystick_connected) {
            return;
        }

        bool pressed = action == GLFW_PRESS;
        std::lock_guard lock(device.m_key_mutex);

        switch(key) {
            case GLFW_KEY_UP    : device.m_pressed[emu::KeypadInput::UP] = pressed; break;
            case GLFW_KEY_DOWN  : device.m_pressed[emu::KeypadInput::DOWN] = pressed; break;
            case GLFW_KEY_LEFT  : device.m_pressed[emu::KeypadInput::LEFT] = pressed; break;
            case GLFW_KEY_RIGHT : device.m_pressed[emu::KeypadInput::RIGHT] = pressed; break;
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