#pragma once

#include "Window.hpp"


class MetricsWindow final : public ui::Window {
public:

    MetricsWindow();

    void setHostTimes(const float *times);
    void setEmulatorTimes(const float *times);

private:

    float m_host_ms[100];
    float m_emu_ms[100];

    void draw() override;
};