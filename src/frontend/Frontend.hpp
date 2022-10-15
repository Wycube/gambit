#pragma once

#include "emulator/core/GBA.hpp"
#include "device/GLFWInputDevice.hpp"
#include "device/OGLVideoDevice.hpp"
#include "DebuggerUI.hpp"
#include "Types.hpp"
#include <miniaudio.h>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <deque>


class EmuThread final {
public:

    EmuThread(emu::GBA &core);
    ~EmuThread();

    void start();
    void stop();
    void pause();

    auto running() const -> bool;
    void runNext();
    auto getClockSpeed() const -> u64;

private:

    emu::GBA &m_core;

    std::thread m_thread;
    std::atomic<bool> m_running;

    std::condition_variable m_cv;
    std::mutex m_mutex;
    bool m_run;
    s32 m_cycle_diff;

    std::chrono::steady_clock::time_point m_start;
    u64 m_clock_start;
    std::atomic<u64> m_clock_speed;
};

class Frontend final {
public:

    Frontend(GLFWwindow *window);

    void init();
    void shutdown();
    void loadROM(std::vector<u8> &&rom);
    void loadBIOS(const std::vector<u8> &bios);

    void drawInterface();

    static void audio_sync(ma_device *device, void *output, const void *input, ma_uint32 frame_count);

private:

    void beginFrame();
    void endFrame();

    static void windowSizeCallback(GLFWwindow *window, int width, int height);

    CallbackUserData m_user_data;
    OGLVideoDevice m_video_device;
    GLFWInputDevice m_input_device;
    emu::GBA m_core;
    EmuThread m_emu_thread;

    DebuggerUI m_debug_ui;
    bool m_show_cpu_debug;
    bool m_show_disasm_debug;
    bool m_show_bkpt_debug;
    bool m_show_scheduler_debug;
    bool m_show_about;
    bool m_show_pak_info;

    std::deque<float> m_frame_times;
    std::chrono::time_point<std::chrono::steady_clock> m_start;

    GLFWwindow *m_window;
    int m_width, m_height;
};