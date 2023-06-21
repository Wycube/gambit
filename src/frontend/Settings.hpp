#pragma once

#include "common/INIParser.hpp"
#include "common/Log.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cstring>
#include <string>
#include <filesystem>


//TODO: Implement better remapping for gamepads (i.e. dpad and axis for a single input)

static const char *BUTTON_NAMES[10] = {
    "button_a", "button_b", "select", "start", "right", "left", "up", "down", "button_l", "button_r"
};

constexpr int default_key_map[10] = {
    GLFW_KEY_A, GLFW_KEY_S,
    GLFW_KEY_X, GLFW_KEY_W,
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
    bool force_integer_scale = false;

    std::string rom_path;
    std::string bios_path;
    bool skip_bios = true;

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

    void loadConfigFile() {
        common::IniMap config = common::loadIniFile(std::filesystem::current_path().string() + "/config.ini");

        if(config.sections.empty() || config.sections.count("settings") == 0) {
            LOG_WARNING("Failed to load 'config.ini' file!");
            return;
        }

        size_t settings_section = config.sections["settings"];

        if(config.values[settings_section].count("show_status_bar") != 0) {
            show_status_bar = config.values[settings_section]["show_status_bar"] == "true";
        }
        if(config.values[settings_section].count("force_integer_scale") != 0) {
            force_integer_scale = config.values[settings_section]["force_integer_scale"] == "true";
        }
        if(config.values[settings_section].count("rom_path") != 0) {
            rom_path = config.values[settings_section]["rom_path"];
        }
        if(config.values[settings_section].count("bios_path") != 0) {
            bios_path = config.values[settings_section]["bios_path"];
        }
        if(config.values[settings_section].count("skip_bios") != 0) {
            skip_bios = config.values[settings_section]["skip_bios"] == "true";
        }

        //Load button maps
        for(int i = 0; i < 10; i++) {
            std::string key_name = std::string("key/") + BUTTON_NAMES[i];
            
            if(config.values[settings_section].count(key_name) != 0) {
                key_map[i] = std::stoi(config.values[settings_section][key_name]);
            }
        }
    }

    void writeConfigFile() {
        common::IniMap config{};

        config.sections["settings"] = 0;
        config.values.push_back({});
        config.values[0]["show_status_bar"] = show_status_bar ? "true" : "false";
        config.values[0]["force_integer_scale"] = force_integer_scale ? "true" : "false";
        config.values[0]["rom_path"] = rom_path;
        config.values[0]["bios_path"] = bios_path;
        config.values[0]["skip_bios"] = skip_bios ? "true" : "false";
        config.values[0]["enable_debugger"] = enable_debugger ? "true" : "false";

        //Write button maps
        for(int i = 0; i < 10; i++) {
            std::string key_name = std::string("key/") + BUTTON_NAMES[i];
            config.values[0][key_name] = std::to_string(key_map[i]);
        }

        common::writeIniFile(config, std::filesystem::current_path().string() + "/config.ini");
    }

    auto operator==(const Settings &other) -> bool {
        return show_status_bar == other.show_status_bar &&
            show_menu_bar == other.show_menu_bar &&
            force_integer_scale == other.force_integer_scale &&
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