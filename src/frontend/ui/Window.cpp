#include "Window.hpp"
#include "frontend/Frontend.hpp"
#include "common/Version.hpp"
#include "common/File.hpp"
#include "common/Types.hpp"
#include "common/Log.hpp"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>


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

    ImGui::SetNextWindowSize(ImVec2{608.0f, 311.0f}, ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Settings", &active)) {
        if(ImGui::IsWindowAppearing()) {
            just_selected = true;
        }

        Settings prev_settings = frontend.getSettings();
        Settings new_settings = frontend.getSettings();

        static int current_item;
        static const char *categories[] = {"General", "Input"};
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
            case 1 : drawInput(new_settings, frontend.getWindow()); break;
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
    ImGui::Text("ROM/BIOS");
    ImGui::Separator();

    if(just_selected) {
        rom_path = settings.rom_path;
        bios_path = settings.bios_path;
    }

    ImGui::Text("ROMs Folder");
    ImGui::BeginTable("##GeneralSettingsTable", 2);
    ImGui::TableSetupColumn("##InputColumn", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("##SetColumn", ImGuiTableColumnFlags_WidthFixed);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##ROMsPath", &rom_path);
    ImGui::TableNextColumn();
    if(ImGui::Button("Set##ROMsFolder")) {
        settings.rom_path = rom_path;
    }
    ImGui::EndTable();

    ImGui::Text("BIOS Path");
    ImGui::BeginTable("##GeneralSettingsTable", 2);
    ImGui::TableSetupColumn("##InputColumn", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("##SetColumn", ImGuiTableColumnFlags_WidthFixed);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##BIOSPath", &bios_path);
    ImGui::TableNextColumn();
    if(ImGui::Button("Set##BIOSPath")) {
        settings.bios_path = bios_path;
    }
    ImGui::EndTable();

    ImGui::Checkbox(" Skip BIOS Intro", &settings.skip_bios);

    ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight()));
    ImGui::Text("Debug");
    ImGui::Separator();

    ImGui::Checkbox("  Enable Debugger", &settings.enable_debugger);
}

auto getInputName(const Settings &settings, int index) -> std::string {
    if(settings.input_source == 0) {
        if(glfwGetKeyName(settings.key_map[index], 0) != nullptr) {
            return glfwGetKeyName(settings.key_map[index], 0);
        } else {
            return std::to_string(settings.key_map[index]);
        }
    } else {
        if(settings.gamepad_map[index].is_button) {
            return fmt::format("Button {}", std::to_string(settings.gamepad_map[index].id));
        } else {
            return fmt::format("{}Axis {}", settings.gamepad_map[index].positive ? '+' : '-', settings.gamepad_map[index].id);
        }
        return std::to_string(settings.gamepad_map[index].id);
    }
}

void inputMapButton(const std::string &label, const std::string &name, int index, bool &open_popup, int &selected_input) {
    if(ImGui::Button(name.c_str(), ImVec2{ImGui::CalcTextSize("_______").x, 0.0f})) {
        open_popup = true;
        selected_input = index;
    }
    ImGui::SameLine();
    ImGui::Text("%s", label.c_str());
}

void SettingsWindow::drawInput(Settings &settings, GLFWwindow *window) {
    std::string sources = "Keyboard\0";
    std::vector<int> sources_list = {0};
    bool fix_list = false;

    for(int i = 0; i < GLFW_JOYSTICK_LAST; i++) {
        if(glfwJoystickPresent(i) && glfwJoystickIsGamepad(i)) {
            fix_list = true;
            sources += '\0';
            sources += fmt::format("{}", glfwGetGamepadName(i));
            sources_list.push_back(i);
        }
    }

    if(fix_list) {
        sources += '\0';
    }

    ImGui::Text("Input Source:");
    ImGui::SameLine();
    ImGui::Combo("##InputSource", &settings.input_source, sources.c_str());
    ImGui::Separator();

    bool open_popup = false;
    ImGui::BeginTable("##InputMappingTable", 4, ImGuiTableFlags_SizingFixedFit);
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    inputMapButton("L", getInputName(settings, 9), 9, open_popup, selected_input);
    ImGui::TableNextColumn();
    inputMapButton("R", getInputName(settings, 8), 8, open_popup, selected_input);

    ImGui::TableNextColumn();
    inputMapButton("UP", getInputName(settings, 6), 6, open_popup, selected_input);
    ImGui::TableNextColumn();
    inputMapButton("DOWN", getInputName(settings, 7), 7, open_popup, selected_input);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    inputMapButton("A", getInputName(settings, 0), 0, open_popup, selected_input);
    ImGui::TableNextColumn();
    inputMapButton("B", getInputName(settings, 1), 1, open_popup, selected_input);

    ImGui::TableNextColumn();
    inputMapButton("LEFT", getInputName(settings, 5), 5, open_popup, selected_input);
    ImGui::TableNextColumn();
    inputMapButton("RIGHT", getInputName(settings, 4), 4, open_popup, selected_input);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    inputMapButton("SELECT", getInputName(settings, 2), 2, open_popup, selected_input);
    ImGui::TableNextColumn();
    inputMapButton("START", getInputName(settings, 3), 3, open_popup, selected_input);

    ImGui::EndTable();

    //Gamepad-only Settings
    if(settings.input_source > 0) {
        ImGui::Spacing();
        ImGui::Text("Stick Dead Zone");
        ImGui::SliderFloat("##StickDeadZoneSlider", &settings.stick_deadzone, 0.0f, 1.0f);
        ImGui::Text("Trigger Dead Zone");
        ImGui::SliderFloat("##TriggerDeadZoneSlider", &settings.trigger_deadzone, 0.0f, 1.0f);
    }

    if(open_popup) {
        ImGui::OpenPopup("Input Binding");
        LOG_INFO("Popup opened");
    }

    static bool dummy;
    dummy = true;
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if(ImGui::BeginPopupModal("Input Binding", &dummy, ImGuiWindowFlags_AlwaysAutoResize)) {
        if(settings.input_source == 0) {
            ImGui::Text("  Press a Key...  ");

            for(int key = 0; key < GLFW_KEY_LAST; key++) {
                if(glfwGetKey(window, key) == GLFW_PRESS) {
                    settings.key_map[selected_input] = key;
                    ImGui::CloseCurrentPopup();
                    break;
                }
            }
        } else {
            ImGui::Text("  Make new input...  ");

            bool finished = false;
            GLFWgamepadstate state;
            glfwGetGamepadState(sources_list[settings.input_source], &state);
        
            //Check all buttons
            for(int button = 0; button < GLFW_GAMEPAD_BUTTON_LAST; button++) {
                if(state.buttons[button]) {
                    settings.gamepad_map[selected_input] = {true, button};
                    finished = true;
                    break;
                }
            }

            //Check all axes
            if(!finished) {
                for(int axis = 0; axis < GLFW_GAMEPAD_AXIS_LAST; axis++) {
                    if(state.axes[axis] > 0.5f) {
                        settings.gamepad_map[selected_input] = {false, axis, true};
                        finished = true;
                        break;
                    } else if(axis != GLFW_GAMEPAD_AXIS_LEFT_TRIGGER && axis != GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER && state.axes[axis] < -0.5f) {
                        settings.gamepad_map[selected_input] = {false, axis, false};
                        finished = true;
                        break;
                    }
                }
            }
            
            if(finished) {
                ImGui::CloseCurrentPopup();
            }
        }
        
        ImGui::EndPopup();
    }
}

} //namespace ui