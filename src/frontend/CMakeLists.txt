add_executable(main main.cpp Frontend.cpp device/OGLVideoDevice.cpp device/GLFWInputDevice.cpp device/MAAudioDevice.cpp
ui/AboutWindow.cpp ui/MetricsWindow.cpp ui/Common.cpp)
target_link_libraries(main PUBLIC gba-lib glfw glad imgui miniaudio)