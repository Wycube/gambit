#include "Common.hpp"


namespace ui {

static ImVector<ImRect> s_GroupPanelLabelStack;

void BeginGroupPanel(const char *name) {
    ImGui::BeginGroup();

    ImVec2 label_size = ImGui::CalcTextSize(name);
    ImVec2 label_min = ImGui::GetWindowPos() + ImGui::GetCursorPos() + ImVec2(15.0f, -label_size.y / 2.0f);

    ImGui::SetCursorPos(ImGui::GetCursorPos() + ImGui::GetStyle().FramePadding);

    if(strcmp(name, "") != 0) {
        ImGui::GetWindowDrawList()->AddText(label_min, ImGui::GetColorU32(ImGuiCol_Text), name);
        s_GroupPanelLabelStack.push_back(ImRect(label_min, label_min + label_size));
    } else {
        s_GroupPanelLabelStack.push_back(ImRect(ImVec2(0, 0), ImVec2(0, 0)));
    }
}

void EndGroupPanel() {
    ImGui::EndGroup();

    float thickness = ImGui::GetStyle().FrameBorderSize;
    float rounding = ImGui::GetStyle().FrameRounding;
    ImVec2 padding = ImGui::GetStyle().FramePadding;
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax() + padding;
    ImRect label_rect = s_GroupPanelLabelStack.back();
    s_GroupPanelLabelStack.pop_back();

    ImGui::Dummy(ImVec2(0.0f, padding.y));

    ImGui::GetWindowDrawList()->PushClipRect(ImVec2(min.x, label_rect.Min.y), ImVec2(label_rect.Min.x - 4.0f, label_rect.Max.y));
    ImGui::GetWindowDrawList()->AddRect(min, max, ImGui::GetColorU32(ImGuiCol_Border), rounding, ImDrawFlags_None, thickness);
    ImGui::GetWindowDrawList()->PopClipRect();
    ImGui::GetWindowDrawList()->PushClipRect(ImVec2(min.x, label_rect.Max.y), max);
    ImGui::GetWindowDrawList()->AddRect(min, max, ImGui::GetColorU32(ImGuiCol_Border), rounding, ImDrawFlags_None, thickness);
    ImGui::GetWindowDrawList()->PopClipRect();
    ImGui::GetWindowDrawList()->PushClipRect(ImVec2(label_rect.Max.x + 4.0f, label_rect.Min.y), ImVec2(max.x, label_rect.Max.y));
    ImGui::GetWindowDrawList()->AddRect(min, max, ImGui::GetColorU32(ImGuiCol_Border), rounding, ImDrawFlags_None, thickness);
    ImGui::GetWindowDrawList()->PopClipRect();
}

} //namespace ui