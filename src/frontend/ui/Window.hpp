#pragma once

#include "frontend/Settings.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

class Frontend;

namespace ui {

struct Window {
    bool active = false;

    virtual void draw(Frontend &frontend) = 0;
};

struct AboutDialog final : public Window {
    void open();
    void draw(Frontend &frontend) override;

private:

    bool open_popup = false;
};

struct FileDialog final : public Window {
    void open();
    void draw(Frontend &frontend) override;

private:

    bool open_popup = false;
};

struct MetricsWindow final : public Window {
    void draw(Frontend &frontend) override;

    void setGUIFrameTimes(const float *values);
    void setGBAFrameTimes(const float *values);
    void setAudioBufferSizes(const float *values);
    void setCPUUsage(const float *values);

private:

    float gui_frame_times[100];
    float gba_frame_times[100];
    float audio_buffer_sizes[100];
    float cpu_usage[100];
};

struct SettingsWindow : public Window {
    void draw(Frontend &frontend) override;

private:

    void drawGeneral(Settings &settings);
    void drawInput(Settings &settings, GLFWwindow *window);

    bool just_selected = false;
    std::string rom_path;
    std::string bios_path;
    int selected_input;
};

} //namespace ui