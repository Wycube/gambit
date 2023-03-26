#include "Window.hpp"
#include "frontend/Frontend.hpp"
#include "common/Version.hpp"
#include "common/File.hpp"
#include "common/Types.hpp"
#include "common/Log.hpp"
#include <imgui.h>


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

} //namespace ui