#include "MetricsWindow.hpp"
#include "Common.hpp"
#include <algorithm>
#include <cstring>


MetricsWindow::MetricsWindow() {
    m_name = "Metrics";
    m_active = false;
}

void MetricsWindow::setHostTimes(const float *times) {
    std::memcpy(m_host_ms, times, sizeof(m_host_ms));
}

void MetricsWindow::setEmulatorTimes(const float *times) {
    std::memcpy(m_emu_ms, times, sizeof(m_emu_ms));
}

void MetricsWindow::draw() {
    if(ImGui::Begin(m_name, &m_active)) {
        ImVec2 plot_size = ImGui::GetContentRegionAvail();
        plot_size.y = std::min(100.0f, plot_size.y / 2.0f) - ImGui::GetStyle().FramePadding.y;

        ImGui::PlotLines("##Host_Frames", m_host_ms, 100, 0, "Host Frame Times", 33.3f, 0.0f, plot_size);
        ImGui::PlotLines("##Emulator_Frames", m_emu_ms, 100, 0, "Emulator Frame Times", 33.3f, 0.0f, plot_size);
    }
    ImGui::End();
}