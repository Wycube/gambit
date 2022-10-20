#pragma once

#include "emulator/device/InputDevice.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <mutex>


class GLFWInputDevice final : public emu::InputDevice {
public:

    GLFWInputDevice(GLFWwindow *window);

    void update();
    auto getKeys() -> u16 override;

private:

    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void joystickConfigCallback(int jid, int event);

    std::mutex m_key_mutex;
    int m_current_joystick;
    bool m_joystick_connected;
};