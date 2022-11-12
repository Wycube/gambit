#include "AboutWindow.hpp"
#include "common/Version.hpp"
#include <imgui.h>
#include <imgui_internal.h>


AboutWindow::AboutWindow() {
    m_name = "About";
    m_active = false;
}

void AboutWindow::draw() {
    ImGui::OpenPopup(m_name);

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if(ImGui::BeginPopupModal(m_name, &m_active, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::Text("Game Boy Advance Emulator,");
        ImGui::Text("Copyright (c) 2021-2022 Wycube");
        ImGui::Separator();
        ImGui::Text("Version: %s", common::GIT_DESC);
        ImGui::Text("Commit: %s", common::GIT_COMMIT);
        ImGui::Text("Branch: %s", common::GIT_BRANCH);
    }
    ImGui::EndPopup();
}