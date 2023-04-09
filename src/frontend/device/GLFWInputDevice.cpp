#include "GLFWInputDevice.hpp"
#include "emulator/core/GBA.hpp"
#include "frontend/Types.hpp"
#include "common/Log.hpp"
#include <cstring>


GLFWInputDevice::GLFWInputDevice(GLFWwindow *window) {
    LOG_DEBUG("Initializing GLFWInputDevice...");

    glfwSetKeyCallback(window, keyCallback);
    joystick_connected = false;
    memset(pressed, 0, sizeof(pressed));
    keyinput.store(0x3FF);

    // tas_input.store(0x3FF);
    // frame_counter = 0;
    // // movie = movie::loadBK2Movie("C:/Users/sburt/Desktop/fusion_tas.bk2");
    // movie = movie::loadVBMMovie("advance.vbm");

    // lagged.store(true);
    // lag_counter = 0;
}

//TODO: This can be done in a better way
void GLFWInputDevice::onFrameStart() {
    // u32 frame = frame_counter - lag_counter;

    // u16 last = tas_input.load();
    // u16 next = ~(movie.inputs[frame * 2] | (movie.inputs[frame * 2 + 1] << 8)) & 0x3FF;
    // // u16 next = ~movie.inputs[frame] & 0x3FF;
    // tas_input.store(next);

    // if(movie.inputs[frame] & (1 << 10)) {
    //     LOG_FATAL("Reset TAS input not implemented!");
    // }

    // if(lagged.load()) {
    //     lag_counter++;
    //     LOG_INFO("Lag Frame");
    // }
    // lagged.store(true);

    // frame_counter++;

    // u16 diff = last ^ next;

    // for(int i = 0; i < 16; i++) {
    //     bool different = (diff >> i) & 1;

    //     if(different) {
    //         bool pressed = (last >> i) & 1;
    //         std::string button = "";

    //         switch(i) {
    //             case 0 : button = "A"; break;
    //             case 1 : button = "B"; break;
    //             case 2 : button = "SELECT"; break;
    //             case 3 : button = "START"; break;
    //             case 4 : button = "RIGHT"; break;
    //             case 5 : button = "LEFT"; break;
    //             case 6 : button = "UP"; break;
    //             case 7 : button = "DOWN"; break;
    //             case 8 : button = "R"; break;
    //             case 9 : button = "L"; break;
    //         }

    //         if(pressed) {
    //             LOG_INFO("{} pressed", button);
    //         } else {
    //             LOG_INFO("{} released", button);
    //         }
    //     }
    // }

    // if(diff) {
    //     callback();
    // }
}

void GLFWInputDevice::update() {
    if(joystick_connected) {
        //Check if still connected
        joystick_connected = glfwJoystickPresent(current_joystick) == GLFW_TRUE;

        if(!joystick_connected) {
            LOG_INFO("Controller in slot {} disconnected: {}", current_joystick, glfwGetJoystickName(current_joystick));
            return;
        }

        //Get input from joystick
        GLFWgamepadstate state;
        glfwGetGamepadState(current_joystick, &state);

        pressed[emu::KeypadInput::UP]       = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_PRESS;
        pressed[emu::KeypadInput::DOWN]     = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_PRESS;
        pressed[emu::KeypadInput::LEFT]     = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] == GLFW_PRESS;
        pressed[emu::KeypadInput::RIGHT]    = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] == GLFW_PRESS;
        pressed[emu::KeypadInput::BUTTON_A] = state.buttons[GLFW_GAMEPAD_BUTTON_B] == GLFW_PRESS;
        pressed[emu::KeypadInput::BUTTON_B] = state.buttons[GLFW_GAMEPAD_BUTTON_A] == GLFW_PRESS;
        pressed[emu::KeypadInput::SELECT]   = state.buttons[GLFW_GAMEPAD_BUTTON_GUIDE] == GLFW_PRESS;
        pressed[emu::KeypadInput::START]    = state.buttons[GLFW_GAMEPAD_BUTTON_START] == GLFW_PRESS;
        pressed[emu::KeypadInput::UP]       = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP];
        pressed[emu::KeypadInput::DOWN]     = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN];
        pressed[emu::KeypadInput::LEFT]     = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT];
        pressed[emu::KeypadInput::RIGHT]    = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT];

        //TODO: Add configurable deadzones
        pressed[emu::KeypadInput::UP]       |= state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] < -0.3f;
        pressed[emu::KeypadInput::DOWN]     |= state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] > 0.3f;
        pressed[emu::KeypadInput::LEFT]     |= state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] < -0.3f;
        pressed[emu::KeypadInput::RIGHT]    |= state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] > 0.3f;
        pressed[emu::KeypadInput::BUTTON_L] = state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > 0.3f;
        pressed[emu::KeypadInput::BUTTON_R] = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.3f;

        u16 keys = 0;
        u16 keys_old = keyinput.load();
        for(size_t i = 0; i < 10; i++) {
            keys |= !pressed[i] << i;
        }
        keyinput.store(keys);

        if(keys_old ^ keys) {
            callback();
        }
    } else {
        //Check for connections
        for(size_t i = 0; i < GLFW_JOYSTICK_LAST; i++) {
            if(glfwJoystickPresent(i) == GLFW_TRUE && glfwJoystickIsGamepad(i)) {
                LOG_INFO("New controller connection in slot {}: {}/{}", i, glfwGetJoystickName(i), glfwGetGamepadName(i));
                current_joystick = i;
                joystick_connected = true;
            }
        }
    }
}

auto GLFWInputDevice::getKeys() -> u16 {
    // lagged.store(false);

    // return tas_input.load();
    return keyinput.load();
}

void GLFWInputDevice::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    CallbackUserData *user_data = reinterpret_cast<CallbackUserData*>(glfwGetWindowUserPointer(window));

    if(user_data != nullptr && action != GLFW_REPEAT) {
        GLFWInputDevice &device = dynamic_cast<GLFWInputDevice&>(user_data->core->input_device);

        if(device.joystick_connected) {
            return;
        }

        bool pressed = action == GLFW_PRESS;

        switch(key) {
            case GLFW_KEY_UP    : device.pressed[emu::KeypadInput::UP] = pressed; break;
            case GLFW_KEY_DOWN  : device.pressed[emu::KeypadInput::DOWN] = pressed; break;
            case GLFW_KEY_LEFT  : device.pressed[emu::KeypadInput::LEFT] = pressed; break;
            case GLFW_KEY_RIGHT : device.pressed[emu::KeypadInput::RIGHT] = pressed; break;
            case GLFW_KEY_A     : device.pressed[emu::KeypadInput::BUTTON_A] = pressed; break;
            case GLFW_KEY_S     : device.pressed[emu::KeypadInput::BUTTON_B] = pressed; break;
            case GLFW_KEY_Z     : device.pressed[emu::KeypadInput::START] = pressed; break;
            case GLFW_KEY_X     : device.pressed[emu::KeypadInput::SELECT] = pressed; break;
            case GLFW_KEY_Q     : device.pressed[emu::KeypadInput::BUTTON_L] = pressed; break;
            case GLFW_KEY_W     : device.pressed[emu::KeypadInput::BUTTON_R] = pressed; break;
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