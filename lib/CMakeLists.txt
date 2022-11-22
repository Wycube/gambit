# {fmt}
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/fmt)

# GLFW
set(GLFW_LIBRARY_TYPE "STATIC")
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(glfw)

# Dear ImGui
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/imgui-docking)

# glad
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/glad)

# miniaudio
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/miniaudio)