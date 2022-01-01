#include "common/Version.hpp"
#include "common/Types.hpp"
#include "common/Log.hpp"
#include "common/Pattern.hpp"
#include "common/StringUtils.hpp"
#include "core/GBA.hpp"
#include "DebuggerUI.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <filesystem>
#include <fstream>
#include <vector>


int main(int argc, char *argv[]) {
    if(argc < 2) {
        LOG_ERROR("No ROM file specified!");
        return -1;
    }

    printf("Version: %s\n", common::GIT_DESC);
    printf("Commit: %s\n", common::GIT_COMMIT);
    printf("Branch: %s\n", common::GIT_BRANCH);

    if(glfwInit() == GLFW_FALSE) {
        LOG_ERROR("GLFW failed to initialize!");
        return -1;
    }
    
    GLFWwindow *window = glfwCreateWindow(1080, 720, "gba", 0, 0);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    int version = gladLoadGL(glfwGetProcAddress);
    if(version == 0) {
        LOG_ERROR("Glad failed to initialize!");
        return -1;
    }

    LOG_DEBUG("Loaded OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    std::fstream file(argv[1], std::ios_base::in | std::ios_base::binary);

    if(!file.good()) {
        LOG_ERROR("File not good!");
        return -1;
    }

    size_t size = std::filesystem::file_size(argv[1]);
    LOG_INFO("ROM Size: {}", size);

    std::vector<u8> rom;
    rom.resize(size);
    
    for(size_t i = 0; i < size; i++) {
        rom[i] = static_cast<u8>(file.get());
    }

    emu::GBA gba;
    gba.loadROM(rom);
    gba.step();

    emu::dbg::Debugger &debugger = gba.getDebugger();

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

    DebuggerUI debug_ui(debugger, gba);
    bool show_debug = true;

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::BeginMainMenuBar();

        if(ImGui::BeginMenu("View")) {
            if(ImGui::MenuItem("Debugger")) {
                show_debug = true;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();

        debug_ui.draw(&show_debug);

        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}