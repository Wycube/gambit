add_executable(main main.cpp Frontend.cpp EmuThread.cpp device/OGLVideoDevice.cpp device/GLFWInputDevice.cpp device/MAAudioDevice.cpp 
ui/Common.cpp ui/Window.cpp)
target_link_libraries(main PUBLIC gba-lib glfw glad imgui miniaudio)
set_property(TARGET main PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)