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

    std::string bios_path;

    if(argc < 3) {
        LOG_WARNING("No BIOS path specified! Using default: bios.bin");
        bios_path = "bios.bin";
    } else {
        bios_path = argv[2];
    }

    printf("Version: %s\n", common::GIT_DESC);
    printf("Commit: %s\n", common::GIT_COMMIT);
    printf("Branch: %s\n", common::GIT_BRANCH);

    if(glfwInit() == GLFW_FALSE) {
        LOG_ERROR("GLFW failed to initialize!");
        return -1;
    }
    
    GLFWwindow *window = glfwCreateWindow(1080, 720, fmt::format("gba  {}", common::GIT_DESC).c_str(), 0, 0);
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

    std::fstream rom_file(argv[1], std::ios_base::in | std::ios_base::binary);
    std::fstream bios_file(bios_path, std::ios_base::in | std::ios_base::binary);

    if(!rom_file.good()) {
        LOG_ERROR("ROM file not good!");
        return -1;
    }

    if(!bios_file.good()) {
        LOG_ERROR("BIOS file not good!");
        return -1;
    }

    size_t rom_size = std::filesystem::file_size(argv[1]);
    size_t bios_size = std::filesystem::file_size(bios_path);
    LOG_INFO("ROM Size: {}", rom_size);
    LOG_INFO("BIOS Size: {}", bios_size);

    std::vector<u8> rom;
    rom.resize(rom_size);
    std::vector<u8> bios;
    bios.resize(bios_size);
    
    for(size_t i = 0; i < rom_size; i++) {
        rom[i] = static_cast<u8>(rom_file.get());
    }

    for(size_t i = 0; i < bios_size; i++) {
        bios[i] = static_cast<u8>(bios_file.get());
    }

    emu::GBA gba;
    gba.loadROM(rom);
    gba.loadBIOS(bios);

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

    DebuggerUI debug_ui(gba);
    bool show_cpu_debug = true;
    bool show_mem_debug = false;
    bool show_vram_debug = false;
    bool show_about = false;
    bool show_pak_info = false;

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if(debug_ui.running()) {
            gba.step(200);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::BeginMainMenuBar();

        if(ImGui::BeginMenu("Debug")) {
            if(ImGui::MenuItem("CPU")) {
                show_cpu_debug = true;
            }
            if(ImGui::MenuItem("Memory Viewer")) {
                show_mem_debug = true;
            }
            if(ImGui::MenuItem("Framebuffer")) {
                show_vram_debug = true;
            }
            if(ImGui::MenuItem("Cart Info")) {
                show_pak_info = true;
            }

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Help")) {
            if(ImGui::MenuItem("About")) {
                show_about = true;
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();

        if(show_cpu_debug) {
            if(ImGui::Begin("CPU Debugger", &show_cpu_debug)) debug_ui.drawCPUDebugger();
            ImGui::End();
        }

        if(show_mem_debug) {
            if(ImGui::Begin("Memory Viewer", &show_mem_debug)) debug_ui.drawMemoryViewer();
            ImGui::End();
        }

        if(show_vram_debug) {
            if(ImGui::Begin("Framebuffer", &show_vram_debug)) debug_ui.drawPPUState();
            ImGui::End();
        }

        if(show_pak_info) {
            if(ImGui::Begin("Pak Info", &show_pak_info)) {
                ImGui::Text("Name: %s", argv[1]);
                ImGui::Text("Size: %u bytes", gba.getGamePak().size());
            }

            ImGui::End();
        }

        if(show_about) {
            if(ImGui::Begin("About", &show_about)) {
                ImGui::Text("Game Boy Advance Emulator,");
                ImGui::Text("Copyright (c) 2021 Wycube");
                ImGui::Separator();
                ImGui::Text("Version: %s", common::GIT_DESC);
                ImGui::Text("Commit: %s", common::GIT_COMMIT);
                ImGui::Text("Branch: %s", common::GIT_BRANCH);
            }

            ImGui::End();
        }

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