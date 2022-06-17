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


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    emu::GBA *gba = reinterpret_cast<emu::GBA*>(glfwGetWindowUserPointer(window));
    
    u16 pressed = ~gba->getKeypad().get_keys();

    if(action == GLFW_PRESS) {
        switch(key) {
            case GLFW_KEY_UP : pressed |= emu::KeypadInput::UP; break;
            case GLFW_KEY_DOWN : pressed |= emu::KeypadInput::DOWN; break;
            case GLFW_KEY_RIGHT : pressed |= emu::KeypadInput::RIGHT; break;
            case GLFW_KEY_LEFT : pressed |= emu::KeypadInput::LEFT; break;
            case GLFW_KEY_A : pressed |= emu::KeypadInput::BUTTON_A; break;
            case GLFW_KEY_S : pressed |= emu::KeypadInput::BUTTON_B; break;
            case GLFW_KEY_Z : pressed |= emu::KeypadInput::START; break;
            case GLFW_KEY_X : pressed |= emu::KeypadInput::SELECT; break;
        }
    }

    if(action == GLFW_RELEASE) {
        switch(key) {
            case GLFW_KEY_UP : pressed &= ~emu::KeypadInput::UP; break;
            case GLFW_KEY_DOWN : pressed &= ~emu::KeypadInput::DOWN; break;
            case GLFW_KEY_RIGHT : pressed &= ~emu::KeypadInput::RIGHT; break;
            case GLFW_KEY_LEFT : pressed &= ~emu::KeypadInput::LEFT; break;
            case GLFW_KEY_A : pressed &= ~emu::KeypadInput::BUTTON_A; break;
            case GLFW_KEY_S : pressed &= ~emu::KeypadInput::BUTTON_B; break;
            case GLFW_KEY_Z : pressed &= ~emu::KeypadInput::START; break;
            case GLFW_KEY_X : pressed &= ~emu::KeypadInput::SELECT; break;
        }
    }


    gba->getKeypad().set_keys(pressed);
}


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
    glfwSwapInterval(0);

    //Setup input callback
    glfwSetKeyCallback(window, key_callback);

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

    //Close stream
    rom_file.close();
    bios_file.close();

    emu::GBA gba;
    gba.loadROM(rom);
    gba.loadBIOS(bios);

    glfwSetWindowUserPointer(window, &gba);

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

    DebuggerUI debug_ui(gba);
    bool show_cpu_debug = true;
    bool show_mem_debug = false;
    bool show_vram_debug = false;
    bool show_scheduler_debug = false;
    bool show_about = false;
    bool show_pak_info = false;

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        for(int i = 0; i < 500; i++) {
            if(debug_ui.running()) {
                gba.step();
            }
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
            if(ImGui::MenuItem("Scheduler")) {
                show_scheduler_debug = true;
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

        if(show_scheduler_debug) {
            if(ImGui::Begin("Scheduler", &show_scheduler_debug)) debug_ui.drawSchedulerViewer();
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

        ImGui::SetNextWindowPos(ImVec2(5.0f, ImGui::GetIO().DisplaySize.y - ImGui::GetTextLineHeight() * 3));
        ImGui::SetNextWindowBgAlpha(0.5f);
        if(ImGui::Begin("##InfoPanel_Window", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs)) {
            ImGui::LabelText("##FPS_Label", "FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
        }
        ImGui::End();

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