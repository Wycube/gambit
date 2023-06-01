#include "GLFWInputDevice.hpp"
#include "emulator/core/GBA.hpp"
#include "frontend/Frontend.hpp"
#include "frontend/Types.hpp"
#include "common/Log.hpp"
#include <cstring>


GLFWInputDevice::GLFWInputDevice(GLFWwindow *window) {
    LOG_DEBUG("Initializing GLFWInputDevice...");

    glfwSetKeyCallback(window, keyCallback);
    joystick_connected = false;
    memset(pressed, 0, sizeof(pressed));
    keyinput.store(0x3FF);
}

auto GLFWInputDevice::update(const Settings &settings) -> bool {
    if(!joystick_connected) {
        return false;
    }

    //Get input from joystick
    GLFWgamepadstate state;
    if(!glfwGetGamepadState(current_joystick, &state)) {
        LOG_INFO("Controller in slot {} disconnected", current_joystick);
        joystick_connected = false;
        keyinput.store(0x3FF);
        return true;
    }

    auto *map = settings.gamepad_map;

    for(int i = 0; i < 10; i++) {
        if(map[i].is_button) {
            pressed[i] = state.buttons[map[i].id];
        } else {
            if(map[i].id == GLFW_GAMEPAD_AXIS_LEFT_TRIGGER || map[i].id == GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER) {
                pressed[i] = state.axes[map[i].id] >= settings.trigger_deadzone;
            } else {
                if(map[i].positive) {
                    pressed[i] = state.axes[map[i].id] >= settings.stick_deadzone;
                } else {
                    pressed[i] = state.axes[map[i].id] <= -settings.stick_deadzone;
                }
            }
        }
    }

    // pressed[emu::KeypadInput::UP]       = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP];
    // pressed[emu::KeypadInput::DOWN]     = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN];
    // pressed[emu::KeypadInput::LEFT]     = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT];
    // pressed[emu::KeypadInput::RIGHT]    = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT];
    // pressed[emu::KeypadInput::BUTTON_A] = state.buttons[GLFW_GAMEPAD_BUTTON_B];
    // pressed[emu::KeypadInput::BUTTON_B] = state.buttons[GLFW_GAMEPAD_BUTTON_A];
    // pressed[emu::KeypadInput::SELECT]   = state.buttons[GLFW_GAMEPAD_BUTTON_GUIDE];
    // pressed[emu::KeypadInput::START]    = state.buttons[GLFW_GAMEPAD_BUTTON_START];

    // pressed[emu::KeypadInput::UP]      |= state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] < -settings.stick_deadzone;
    // pressed[emu::KeypadInput::DOWN]    |= state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] > settings.stick_deadzone;
    // pressed[emu::KeypadInput::LEFT]    |= state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] < -settings.stick_deadzone;
    // pressed[emu::KeypadInput::RIGHT]   |= state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] > settings.stick_deadzone;
    // pressed[emu::KeypadInput::BUTTON_L] = state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > settings.trigger_deadzone;
    // pressed[emu::KeypadInput::BUTTON_R] = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > settings.trigger_deadzone;

    u16 keys = 0;
    u16 keys_old = keyinput.load();
    for(size_t i = 0; i < 10; i++) {
        keys |= !pressed[i] << i;
    }
    keyinput.store(keys);

    if(keys_old ^ keys) {
        callback();
    }

    return false;
}

void GLFWInputDevice::updateInputSource(int source) {
    //Keyboard is source 0
    joystick_connected = source > 0;
    
    if(joystick_connected) {
        //Find the gamepad being used
        int num_connected = 0;
        for(size_t i = 0; i < GLFW_JOYSTICK_LAST; i++) {
            if(glfwJoystickPresent(i) && glfwJoystickIsGamepad(i) && ++num_connected == source - 1) {
                joystick_connected = i;
                break;
            }
        }
    }
}

void GLFWInputDevice::setActive(bool active) {
    this->active = active;

    if(!active) {
        keyinput.store(0x3FF);
    }
}

auto GLFWInputDevice::isActive() -> bool {
    return active;
}

auto GLFWInputDevice::getKeys() -> u16 {
    return keyinput.load();
}

void GLFWInputDevice::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    CallbackUserData *user_data = reinterpret_cast<CallbackUserData*>(glfwGetWindowUserPointer(window));

    if(user_data != nullptr && action != GLFW_REPEAT) {
        GLFWInputDevice &device = dynamic_cast<GLFWInputDevice&>(user_data->core->input_device);
        const int *key_map = reinterpret_cast<Frontend*>(user_data->frontend)->getSettings().key_map;

        if(device.joystick_connected) {
            return;
        }

        for(int i = 0; i < 10; i++) {
            if(key == key_map[i]) {
                device.pressed[i] = action == GLFW_PRESS;
            }
        }

        u16 keys = 0;
        u16 keys_old = device.keyinput.load();
        for(size_t i = 0; i < 10; i++) {
            keys |= !device.pressed[i] << i;
        }
        device.keyinput.store(keys);
        
        if(keys_old ^ keys) {
            device.callback();
        }
    }
}