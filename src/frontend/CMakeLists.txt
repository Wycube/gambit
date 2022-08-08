add_executable(main main.cpp Application.cpp device/OGLVideoDevice.cpp device/GLFWInputDevice.cpp)
target_link_libraries(main gba-lib glfw glad imgui)