add_executable(gambit main.cpp Frontend.cpp EmuThread.cpp device/OGLVideoDevice.cpp device/GLFWInputDevice.cpp device/MAAudioDevice.cpp 
ui/Window.cpp)
target_link_libraries(gambit PUBLIC gba-lib glfw glad imgui miniaudio)
set_property(TARGET gambit PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)