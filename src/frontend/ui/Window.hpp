#pragma once

#include "frontend/Settings.hpp"

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
    void drawInput(Settings &settings);
    void drawVideo(Settings &settings);
    void drawAudio(Settings &settings);
    void drawDebug(Settings &settings);

    bool just_selected = false;
    char rom_path_buf[100] = {0};
    char bios_path_buf[100] = {0};
};

} //namespace ui