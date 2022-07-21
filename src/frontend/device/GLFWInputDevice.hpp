#pragma once

#include "emulator/device/InputDevice.hpp"
#include <GLFW/glfw3.h>


class GLFWInputDevice final : public emu::InputDevice {
public:

    GLFWInputDevice(GLFWwindow *window);

    auto getKeys() -> u16 override;

private:

    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
};