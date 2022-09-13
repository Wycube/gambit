#pragma once

#include "emulator/core/GBA.hpp"
#include "device/GLFWInputDevice.hpp"
#include "device/OGLVideoDevice.hpp"
#include "DebuggerUI.hpp"
#include "Types.hpp"
#include <miniaudio.h>
#include <thread>
#include <atomic>


class EmuThread final {
public:

    EmuThread(emu::GBA &core);
    ~EmuThread();

    void start();
    void stop();
    void pause();

    auto running() const -> bool;
    void addCycles(u32 cycles);
    auto getClockSpeed() const -> u64;

private:

    emu::GBA &m_core;

    std::thread m_thread;
    std::atomic<bool> m_running;
    std::atomic<u32> m_cycles_left;
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
    u32 m_sync_counter;

    DebuggerUI m_debug_ui;
    bool m_show_cpu_debug;
    bool m_show_disasm_debug;
    bool m_show_bkpt_debug;
    bool m_show_about;
    bool m_show_pak_info;

    GLFWwindow *m_window;
    int m_width, m_height;
};