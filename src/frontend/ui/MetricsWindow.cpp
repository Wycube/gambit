#include "MetricsWindow.hpp"
#include "Common.hpp"
#include <algorithm>
#include <cstring>


MetricsWindow::MetricsWindow() {
    name = "Metrics";
    active = false;
}

void MetricsWindow::setHostTimes(const float *times) {
    std::memcpy(host_ms, times, sizeof(host_ms));
}

void MetricsWindow::setEmulatorTimes(const float *times) {
    std::memcpy(emu_ms, times, sizeof(emu_ms));
}

void MetricsWindow::draw() {
    if(ImGui::Begin(name, &active)) {
        ImVec2 plot_size = ImGui::GetContentRegionAvail();
        plot_size.y = std::min(100.0f, plot_size.y / 2.0f) - ImGui::GetStyle().FramePadding.y;

        ImGui::PlotLines("##Host_Frames", host_ms, 100, 0, "Host Frame Times", 33.3f, 0.0f, plot_size);
        ImGui::PlotLines("##Emulator_Frames", emu_ms, 100, 0, "Emulator Frame Times", 33.3f, 0.0f, plot_size);
    }
    ImGui::End();
}