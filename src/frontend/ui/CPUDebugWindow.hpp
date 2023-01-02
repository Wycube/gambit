#pragma once

#include "Window.hpp"


class CPUDebugWindow final : public ui::Window {
public:

    CPUDebugWindow();

private:

    void draw() override;
};