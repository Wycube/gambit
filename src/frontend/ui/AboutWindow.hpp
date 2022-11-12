#pragma once

#include "Window.hpp"


class AboutWindow final : public ui::Window {
public:

    AboutWindow();

private:

    void draw() override;
};