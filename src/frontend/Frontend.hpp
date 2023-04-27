#pragma once

#include "EmuThread.hpp"
#include "emulator/core/GBA.hpp"
#include "device/GLFWInputDevice.hpp"
#include "device/OGLVideoDevice.hpp"
#include "device/MAAudioDevice.hpp"
#include "ui/Window.hpp"
#include "Types.hpp"


class Frontend final {
public:

    Frontend(GLFWwindow *window);

    void init();
    void shutdown();
    void mainloop();
    void startEmulation();
    void stopEmulation();
    void resetEmulation();
    void resetAndLoad(const std::string &path);
    auto loadROM(const std::string &path) -> bool;
    void loadBIOS(const std::vector<u8> &bios);
    void refreshGameList();

private:

    void refreshScreenDimensions();
    void drawInterface();
    void beginFrame();
    void endFrame();
    void audioCallback(bool new_samples, float *samples, size_t buffer_size);
    static void windowSizeCallback(GLFWwindow *window, int width, int height);

    CallbackUserData m_user_data;
    OGLVideoDevice m_video_device;
    GLFWInputDevice m_input_device;
    MAAudioDevice m_audio_device;
    std::shared_ptr<emu::GBA> m_core;
    EmuThread m_emu_thread;

    bool rom_loaded;
    std::vector<ui::Window*> m_windows;
    ui::AboutDialog m_about_dialog;
    ui::FileDialog m_file_dialog;
    std::vector<std::string> game_list;

    common::FixedRingBuffer<float, 100> m_frame_times;
    common::FixedRingBuffer<float, 100> m_audio_buffer_size;
    std::mutex m_audio_buffer_mutex;
    float m_audio_samples_l[750];
    float m_audio_samples_r[750];
    std::chrono::time_point<std::chrono::steady_clock> m_start;
    float m_average_fps;
    float m_gba_fps;

    float screen_width, screen_height;

    GLFWwindow *m_window;
    int m_width, m_height;
};