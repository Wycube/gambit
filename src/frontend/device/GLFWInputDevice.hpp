#pragma once

#include "emulator/device/InputDevice.hpp"
#include "emulator/core/movie/BK2Loader.hpp"
#include "emulator/core/movie/VBMLoader.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <atomic>


class GLFWInputDevice final : public emu::InputDevice {
public:

    explicit GLFWInputDevice(GLFWwindow *window);

    void onFrameStart() override;
    void update();
    auto getKeys() -> u16 override;

private:

    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

    std::atomic<u16> keyinput;
    bool pressed[10];
    int current_joystick;
    bool joystick_connected;

    movie::VBMMovie movie;
    // movie::BK2Movie movie;
    std::atomic<u16> tas_input;
    u32 frame_counter;

    std::atomic<bool> lagged;
    u32 lag_counter;
};