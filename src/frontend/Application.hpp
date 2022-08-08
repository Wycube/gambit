#pragma once

#include "emulator/core/GBA.hpp"
#include "device/GLFWInputDevice.hpp"
#include "device/OGLVideoDevice.hpp"
#include "DebuggerUI.hpp"
#include <thread>
#include <atomic>


struct CallbackUserData {
    void *application;
    emu::GBA *core;
};

class Application final {
public:

    Application(GLFWwindow *window);

    void init();
    void shutdown();
    void loadROM(std::vector<u8> &&rom);
    void loadBIOS(const std::vector<u8> &bios);

    void drawInterface();

    //Temporary
    auto getCore() -> emu::GBA* {
        return &m_core;
    }

private:

    void beginFrame();
    void endFrame();

    static void windowSizeCallback(GLFWwindow *window, int width, int height);

    CallbackUserData m_user_data;
    OGLVideoDevice m_video_device;
    GLFWInputDevice m_input_device;
    emu::GBA m_core;
    std::thread m_emu_thread;
    std::atomic<bool> m_stop;

    std::chrono::steady_clock::time_point m_start;
    u64 m_clock_start;
    std::atomic<u64> m_clock_speed;

    DebuggerUI m_debug_ui;
    bool m_show_cpu_debug;
    bool m_show_about;

    GLFWwindow *m_window;
    int m_width, m_height;
};