#pragma once

#include "emulator/device/InputDevice.hpp"
#include "frontend/Settings.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <atomic>


class GLFWInputDevice final : public emu::InputDevice {
public:

    explicit GLFWInputDevice(GLFWwindow *window);

    auto update(const Settings &settings) -> bool;
    void updateInputSource(int source);
    void setActive(bool active);
    auto isActive() -> bool;
    auto getKeys() -> u16 override;

private:

    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

    std::atomic<u16> keyinput;
    bool pressed[10];
    int current_joystick;
    bool joystick_connected;
    bool active;
};