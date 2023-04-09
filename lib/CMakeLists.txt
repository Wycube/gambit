# {fmt}
add_subdirectory(fmt)

# GLFW
set(GLFW_LIBRARY_TYPE "STATIC")
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(glfw)

# Dear ImGui
add_subdirectory(imgui-docking)

# glad
add_subdirectory(glad)

# miniaudio
add_subdirectory(miniaudio)

# zip
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(zip-0.2.6)