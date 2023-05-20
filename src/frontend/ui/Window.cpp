#include "Window.hpp"
#include "Common.hpp"
#include "frontend/Frontend.hpp"
#include "common/Version.hpp"
#include "common/File.hpp"
#include "common/Types.hpp"
#include "common/Log.hpp"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>


namespace ui {

void AboutDialog::open() {
    open_popup = true;
    active = true;
}

void AboutDialog::draw(Frontend&) {
    if(open_popup) {
        ImGui::OpenPopup("About");
        open_popup = false;
    }

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if(ImGui::BeginPopupModal("About", &active, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::Text("Game Boy Advance Emulator,");
        ImGui::Text("Copyright (c) 2021-2023 Wycube");
        ImGui::Separator();
        ImGui::Text("Version: %s", common::GIT_DESC);
        ImGui::Text("Commit: %s", common::GIT_COMMIT);
        ImGui::Text("Branch: %s", common::GIT_BRANCH);
    }
    ImGui::EndPopup();
}

void FileDialog::open() {
    open_popup = true;
    active = true;
}

void FileDialog::draw(Frontend &frontend) {
    static char path_buf[100];

    if(open_popup) {
        ImGui::OpenPopup("Load ROM");
        open_popup = false;
        path_buf[0] = '\0';
    }

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if(ImGui::BeginPopupModal("Load ROM", &active, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        bool entered = ImGui::InputText("Path", path_buf, 100, ImGuiInputTextFlags_EnterReturnsTrue);

        if(ImGui::Button("Load") || entered) {
            if(std::filesystem::exists(path_buf) && std::filesystem::is_regular_file(path_buf)) {
                frontend.resetAndLoad(path_buf);
                ImGui::CloseCurrentPopup();
            }
        }
    }
    ImGui::EndPopup();
}

void MetricsWindow::draw(Frontend &frontend) {
    if(ImGui::Begin("Metrics", &active, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImVec2 size = ImVec2(0.0f, 50.0f);

        ImGui::Text("GUI");
        ImGui::Separator();
        ImGui::PlotLines("##GUI_Frame_Times", gui_frame_times, 100, 0, "GUI Frame Times", 33.3f, 0.0f, size);

        ImGui::Text("GBA");
        ImGui::Separator();
        ImGui::PlotLines("##GBA_Frame_Times", gba_frame_times, 100, 0, "GBA Frame Times", 33.3f, 0.0f, size);
        ImGui::PlotLines("##Audio_Buffer_Sizes", audio_buffer_sizes, 100, 0, "Audio Buffer Size", 0.0f, 8192.0f, size);
        ImGui::PlotHistogram("##CPU_Usage", cpu_usage, 100, 0, "CPU Usage", 0.0, 100.0f, size);
    }
    ImGui::End();
}

void MetricsWindow::setGUIFrameTimes(const float *values) {
    std::memcpy(gui_frame_times, values, sizeof(gui_frame_times));
}

void MetricsWindow::setGBAFrameTimes(const float *values) {
    std::memcpy(gba_frame_times, values, sizeof(gba_frame_times));
}

void MetricsWindow::setAudioBufferSizes(const float *values) {
    std::memcpy(audio_buffer_sizes, values, sizeof(audio_buffer_sizes));
}

void MetricsWindow::setCPUUsage(const float *values) {
    std::memcpy(cpu_usage, values, sizeof(cpu_usage));
}

void SettingsWindow::draw(Frontend &frontend) {
    ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));


    if(ImGui::Begin("Settings", &active)) {
        if(ImGui::IsWindowAppearing()) {
            just_selected = true;
        }

        Settings prev_settings = frontend.getSettings();
        Settings new_settings = frontend.getSettings();

        static int current_item;
        static const char *categories[] = {"General", "Input", "Video", "Audio", "Debug"};
        ImGui::BeginGroup();
        ImGui::SetNextItemWidth(ImGui::CalcTextSize("General______").x);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        if(ImGui::ListBox("##Categories", &current_item, categories, sizeof(categories) / sizeof(decltype(categories[0])))) {
            just_selected = true;
        }
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y));
        ImGui::EndGroup();

        ImGui::SameLine(0, 0);
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

        ImGui::SameLine();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, window_padding);
        ImGui::BeginChild("##SettingsChild", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding);
        
        switch(current_item) {
            case 0 : drawGeneral(new_settings); break;
            case 1 : drawInput(new_settings); break;
            case 2 : drawVideo(new_settings); break;
            case 3 : drawAudio(new_settings); break;
            case 4 : drawDebug(new_settings); break;
        }

        if(just_selected) {
            just_selected = false;
        }

        if(new_settings != prev_settings) {
            frontend.setSettings(new_settings);
            LOG_INFO("Settings Changed");
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    ImGui::PopStyleVar();
}

void SettingsWindow::drawGeneral(Settings &settings) {
    ImGui::Text("UI");
    ImGui::Separator();
    ImGui::Text("Show Status Bar");
    ImGui::SameLine();
    ImGui::Checkbox("##ShowStatusBar", &settings.show_status_bar);

    ImGui::Text("ROM/BIOS");
    ImGui::Separator();

    if(just_selected) {
        rom_path_buf[0] = '\0';
        bios_path_buf[0] = '\0';
        std::memcpy(rom_path_buf, settings.rom_path.data(), 100 < settings.rom_path.size() ? 100 : settings.rom_path.size());
        std::memcpy(bios_path_buf, settings.bios_path.data(), 100 < settings.bios_path.size() ? 100 : settings.bios_path.size());
    }

    ImGui::Text("ROMs Folder:");
    ImGui::SameLine();
    ImGui::InputText("##ROMsFolder", rom_path_buf, 100);
    ImGui::SameLine();
    if(ImGui::Button("Set##ROMsFolder")) {
        settings.rom_path = rom_path_buf;
    }

    ImGui::Text("BIOS Path:");
    ImGui::SameLine();
    ImGui::InputText("##BIOSPath", bios_path_buf, 100);
    ImGui::SameLine();
    if(ImGui::Button("Set##BIOSPath")) {
        settings.bios_path = bios_path_buf;
    }

    ImGui::Text("Skip BIOS Intro");
    ImGui::SameLine();
    ImGui::Checkbox("##SkipBIOSIntro", &settings.skip_bios);
}

void SettingsWindow::drawInput(Settings &settings) {
    ImGui::Text("Input Source:");
    ImGui::SameLine();
    static int aldkwnflkamwdl;
    ImGui::Combo("##InputSource", &aldkwnflkamwdl, "Keyboard\0Controller\0");

    const char *buttons[12] = {"A", "B", "X", "Y", "R", "L", "START", "SELECT", "UP", "DOWN", "LEFT", "RIGHT"};
    for(int i = 0; i < 12; i++) {
        ImGui::Text("%s", buttons[i]);
        ImGui::SameLine();
        ImGui::Button("##aklwdmlakwnlk");
    }
}

void SettingsWindow::drawVideo(Settings &settings) {
    ImGui::Text("Screen Filter");
    ImGui::SameLine();
    ImGui::Combo("##ScreenFilter", &settings.screen_filter, "None\0Linear\0");

    static bool kselkm;
    ImGui::Text("Enabled Layers");
    ImGui::Text("Background 0:");
    ImGui::SameLine();
    ImGui::Checkbox("##Background0", &kselkm);
    ImGui::Text("Background 1:");
    ImGui::SameLine();
    ImGui::Checkbox("##Background1", &kselkm);
    ImGui::Text("Background 2:");
    ImGui::SameLine();
    ImGui::Checkbox("##Background2", &kselkm);
    ImGui::Text("Background 3:");
    ImGui::SameLine();
    ImGui::Checkbox("##Background3", &kselkm);
    ImGui::Text("Objects:");
    ImGui::SameLine();
    ImGui::Checkbox("##Objects", &kselkm);
    ImGui::Text("Effects:");
    ImGui::SameLine();
    ImGui::Checkbox("##Effects", &kselkm);
}

void SettingsWindow::drawAudio(Settings &settings) {
    ImGui::Text("Volume:");
    ImGui::SameLine();
    ImGui::SliderFloat("##Volume", &settings.volume, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp);

    static bool yeet;
    ImGui::Text("Enabled Channels");
    ImGui::Text("Pulse Channel 1:");
    ImGui::SameLine();
    ImGui::Checkbox("##PulseChannel1", &yeet);
    ImGui::Text("Pulse Channel 2:");
    ImGui::SameLine();
    ImGui::Checkbox("##PulseChannel2", &yeet);
    ImGui::Text("Wave Channel:");
    ImGui::SameLine();
    ImGui::Checkbox("##WaveChannel", &yeet);
    ImGui::Text("Noise Channel:");
    ImGui::SameLine();
    ImGui::Checkbox("##NoiseChannel", &yeet);
    ImGui::Text("FIFO A:");
    ImGui::SameLine();
    ImGui::Checkbox("##FIFOA", &yeet);
    ImGui::Text("FIFO B:");
    ImGui::SameLine();
    ImGui::Checkbox("##FIFOB", &yeet);
}

void SettingsWindow::drawDebug(Settings &settings) {
    ImGui::Text("Enable Debugger");
    ImGui::SameLine();
    ImGui::Checkbox("##EnableDebugger", &settings.enable_debugger);
}

} //namespace ui