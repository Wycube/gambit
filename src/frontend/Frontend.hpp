#pragma once

#include "EmuThread.hpp"
#include "emulator/core/GBA.hpp"
#include "device/GLFWInputDevice.hpp"
#include "device/OGLVideoDevice.hpp"
#include "device/MAAudioDevice.hpp"
#include "ui/Window.hpp"
#include "Types.hpp"
#include "Settings.hpp"


class Frontend final {
public:

    explicit Frontend(GLFWwindow *glfw_window);

    void init();
    void shutdown();
    void mainloop();
    void startEmulation();
    void stopEmulation();
    void resetEmulation();
    void resetAndLoad(const std::string &path);
    auto loadROM(const std::string &path) -> bool;
    void loadBIOS(const std::vector<u8> &bios);
    auto getSettings() -> Settings;
    void setSettings(Settings new_settings);

private:

    void refreshScreenDimensions();
    void refreshGameList();
    void drawInterface();
    void beginFrame();
    void endFrame();
    void audioCallback(bool new_samples, float *samples, size_t buffer_size);
    static void windowSizeCallback(GLFWwindow *window, int width, int height);

    GLFWwindow *window;
    int width, height;
    CallbackUserData user_data;
    OGLVideoDevice video_device;
    GLFWInputDevice input_device;
    MAAudioDevice audio_device;
    std::shared_ptr<emu::GBA> core;
    EmuThread emu_thread;

    bool rom_loaded;
    float screen_width, screen_height;
    float frame_height;
    Settings settings;
    std::vector<std::string> game_list;
    std::vector<ui::Window*> ui_windows;
    ui::AboutDialog about_dialog;
    ui::FileDialog file_dialog;
    ui::MetricsWindow metrics_window;
    ui::SettingsWindow settings_window;

    common::FixedRingBuffer<float, 100> frame_times;
    common::FixedRingBuffer<float, 100> audio_buffer_size;
    std::mutex audio_buffer_mutex;
    float audio_samples_l[750];
    float audio_samples_r[750];
    std::chrono::time_point<std::chrono::steady_clock> start;
    float average_fps;
    float gba_fps;
};