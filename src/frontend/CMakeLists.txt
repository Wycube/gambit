add_executable(main main.cpp device/OGLVideoDevice.cpp)
target_link_libraries(main gba-lib glfw glad imgui)