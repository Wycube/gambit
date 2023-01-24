#pragma once

#include "emulator/core/GBA.hpp"
#include "device/GLFWInputDevice.hpp"
#include "device/OGLVideoDevice.hpp"
#include "device/MAAudioDevice.hpp"
#include "DebuggerUI.hpp"
#include "ui/AboutWindow.hpp"
#include "ui/MetricsWindow.hpp"
#include "Types.hpp"
#define MA_NO_ENCODING
#define MA_NO_DECODING
#include <miniaudio.h>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <deque>


enum Command {
    RUN,
    TERMINATE
};

class EmuThread final {
public:

    EmuThread(std::shared_ptr<emu::GBA> core);
    ~EmuThread();

    void start();
    void stop();
    void pause();
    void sendCommand(Command cmd);
    void setFastforward(bool enable);
    auto fastforwarding() -> bool;

    auto running() const -> bool;
    auto getClockSpeed() const -> u64;

private:

    void processCommands();

    std::shared_ptr<emu::GBA> m_core;

    std::thread m_thread;
    std::atomic<bool> m_running;

    std::condition_variable m_cv;
    std::mutex m_mutex;
    s32 m_cycle_diff;

    std::chrono::steady_clock::time_point m_start;
    u64 m_clock_start;
    std::atomic<u64> m_clock_speed;
    common::ThreadSafeRingBuffer<Command, 20> cmd_queue;
    std::atomic<bool> fastforward;
};

class Frontend final {
public:

    Frontend(GLFWwindow *window);

    void init();
    void shutdown();
    void mainloop();
    void loadROM(std::vector<u8> &&rom);
    void loadBIOS(const std::vector<u8> &bios);
    void loadSave(const std::string &filename);
    void writeSave(const std::string &filename);

    void drawInterface();

    static void audio_sync(ma_device *device, void *output, const void *input, ma_uint32 frame_count);

private:

    void beginFrame();
    void endFrame();

    static void windowSizeCallback(GLFWwindow *window, int width, int height);

    CallbackUserData m_user_data;
    OGLVideoDevice m_video_device;
    GLFWInputDevice m_input_device;
    MAAudioDevice m_audio_device;
    std::shared_ptr<emu::GBA> m_core;
    EmuThread m_emu_thread;

    DebuggerUI m_debug_ui;
    AboutWindow m_about_window;
    MetricsWindow m_metrics_window;
    bool m_show_status_bar;
    bool m_show_cpu_debug;
    bool m_show_disasm_debug;
    bool m_show_scheduler_debug;
    bool m_show_pak_info;
    bool m_show_settings;

    common::FixedRingBuffer<float, 100> m_frame_times;
    common::FixedRingBuffer<float, 100> m_audio_buffer_size;
    std::mutex m_audio_buffer_mutex;
    float m_audio_samples_l[750];
    float m_audio_samples_r[750];
    std::chrono::time_point<std::chrono::steady_clock> m_start;
    float m_average_fps;
    float m_gba_fps;

    GLFWwindow *m_window;
    int m_width, m_height;
};