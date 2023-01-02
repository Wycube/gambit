#pragma once

#include "emulator/device/InputDevice.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <atomic>


class GLFWInputDevice final : public emu::InputDevice {
public:

    explicit GLFWInputDevice(GLFWwindow *window);

    void update();
    auto getKeys() -> u16 override;

private:

    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void joystickConfigCallback(int jid, int event);

    std::atomic<u16> keyinput;
    bool pressed[10];
    int current_joystick;
    bool joystick_connected;
};