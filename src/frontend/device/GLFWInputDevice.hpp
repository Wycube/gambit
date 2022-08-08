#pragma once

#include "emulator/device/InputDevice.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <mutex>


class GLFWInputDevice final : public emu::InputDevice {
public:

    GLFWInputDevice(GLFWwindow *window);

    auto getKeys() -> u16 override;

private:

    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

    std::mutex m_key_mutex;
};