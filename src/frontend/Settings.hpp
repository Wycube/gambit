#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cstring>
#include <string>


//TODO: Implement better remapping for gamepads (i.e. dpad and axis for a single input)

constexpr int default_key_map[10] = {
    GLFW_KEY_A, GLFW_KEY_S,
    GLFW_KEY_Z, GLFW_KEY_X,
    GLFW_KEY_RIGHT, GLFW_KEY_LEFT,
    GLFW_KEY_UP, GLFW_KEY_DOWN,
    GLFW_KEY_W, GLFW_KEY_Q
};

struct GamepadInput {
    bool is_button;
    int id;
    bool positive;
};

constexpr GamepadInput default_gamepad_map[10] = {
    {true, GLFW_GAMEPAD_BUTTON_B}, {true, GLFW_GAMEPAD_BUTTON_A},
    {true, GLFW_GAMEPAD_BUTTON_BACK}, {true, GLFW_GAMEPAD_BUTTON_START},
    {false, GLFW_GAMEPAD_AXIS_LEFT_X, true}, {false, GLFW_GAMEPAD_AXIS_LEFT_X, false},
    {false, GLFW_GAMEPAD_AXIS_LEFT_Y, false}, {false, GLFW_GAMEPAD_AXIS_LEFT_Y, true},
    {false, GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER, true}, {false, GLFW_GAMEPAD_AXIS_LEFT_TRIGGER, false}
};

struct Settings {
    bool show_status_bar = false;
    bool show_menu_bar = true;

    std::string rom_path;
    std::string bios_path;
    bool skip_bios = false;

    int input_source = 0;
    int key_map[10];
    GamepadInput gamepad_map[10];
    float stick_deadzone = 0.3f;
    float trigger_deadzone = 0.3f;

    bool enable_debugger = false;
    
    Settings() {
        std::memcpy(key_map, default_key_map, sizeof(key_map));
        std::memcpy(gamepad_map, default_gamepad_map, sizeof(gamepad_map));
    }

    auto operator==(const Settings &other) -> bool {
        return show_status_bar == other.show_status_bar &&
            show_menu_bar == other.show_menu_bar &&
            rom_path == other.rom_path &&
            bios_path == other.bios_path &&
            skip_bios == other.skip_bios &&
            enable_debugger == other.enable_debugger &&
            input_source == other.input_source &&
            std::memcmp(key_map, other.key_map, sizeof(key_map)) == 0 &&
            std::memcmp(gamepad_map, other.gamepad_map, sizeof(gamepad_map)) == 0 &&
            stick_deadzone == other.stick_deadzone &&
            trigger_deadzone == other.trigger_deadzone;
    }

    auto operator!=(const Settings &other) -> bool {
        return !(*this == other);
    }
};