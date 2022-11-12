#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>


namespace ui {

void BeginGroupPanel(const char *name);
void EndGroupPanel();

} //namespace ui