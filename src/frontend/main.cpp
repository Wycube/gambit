#include "core/GBA.hpp"
#include "common/Version.hpp"
#include "common/Types.hpp"
#include "common/Log.hpp"
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
    LOG_INFO("Version: {}", common::GIT_DESC);
    LOG_INFO("Commit: {}", common::GIT_COMMIT);
    LOG_INFO("Branch: {}", common::GIT_BRANCH);

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

    if(glfwInit() == GLFW_FALSE) {
        LOG_ERROR("GLFW failed to initialize!");
        return -1;
    }
    
    GLFWwindow *window = glfwCreateWindow(1080, 720, "", 0, 0);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    //Setup input callback
    glfwSetKeyCallback(window, key_callback);

    int version = gladLoadGL(glfwGetProcAddress);
    if(version == 0) {
        LOG_ERROR("Glad failed to initialize!");
        return -1;
    }

    LOG_INFO("Loaded OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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
    gba.loadBIOS(bios);
    gba.loadROM(std::move(rom));

    char game_title[13];
    std::memcpy(game_title, gba.getGamePak().getHeader().title, 12);
    game_title[12] = '\0';
    glfwSetWindowTitle(window, fmt::format("gba  [{}] - {}", common::GIT_DESC, game_title).c_str());

    glfwSetWindowUserPointer(window, &gba);

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

    DebuggerUI debug_ui(gba);
    bool show_cpu_debug = true;
    bool show_mem_debug = false;
    bool show_vram_debug = false;
    bool show_scheduler_debug = false;
    bool show_about = false;
    bool show_pak_info = false;

    u32 cycles_start = gba.getCurrentTimestamp();
    auto start = std::chrono::steady_clock::now();
    u32 clock_speed = 0;

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if(std::chrono::steady_clock::now() > (start + std::chrono::seconds(1))) {
            start = std::chrono::steady_clock::now();
            clock_speed = gba.getCurrentTimestamp() - cycles_start;
            cycles_start = gba.getCurrentTimestamp();
        }

        for(int i = 0; i < 700; i++) {
            if(debug_ui.running()) {
                gba.step();
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
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

        ImGui::SetNextWindowSize(ImVec2(width, height - ImGui::GetFrameHeight() * 2));
        ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if(ImGui::Begin("##GBA Screen", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration)) {
            debug_ui.drawScreen();
        }
        ImGui::End();
        ImGui::PopStyleVar(2);


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
                emu::GamePak &pak = gba.getGamePak();
                emu::GamePakHeader &header = pak.getHeader();
                ImGui::Text("Size: %u bytes", pak.size());
                ImGui::Text("Name: %s", argv[1]);
                ImGui::Text("Internal Title: %s", game_title);
                ImGui::Text("Game Code: %c%c%c%c", header.game_code[0], header.game_code[1], header.game_code[2], header.game_code[3]);
                ImGui::Text("Maker Code: %c%c", header.maker_code[0], header.maker_code[1]);
                ImGui::Text("Version: %i", header.version);
                ImGui::Text("Device Type: %i", header.device_type);
                ImGui::Text("Main Unit Code: %i", header.unit_code);
                ImGui::Text("Checksum: %i", header.checksum);
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

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        if(ImGui::BeginViewportSideBar("##StatusBar", ImGui::GetMainViewport(), ImGuiDir_Down, ImGui::GetFrameHeight(), ImGuiWindowFlags_MenuBar)) {
            if(ImGui::BeginMenuBar()) {
                ImGui::SetNextItemWidth(ImGui::CalcTextSize("FPS: 0000.0 ").x);
                ImGui::LabelText("##FPS_Label", "FPS: %6.1f", ImGui::GetIO().Framerate);
                ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
                ImGui::SetNextItemWidth(ImGui::CalcTextSize("Clock Speed: 00000000 ").x);
                ImGui::LabelText("##HZ_Label", "Clock Speed: %8i", clock_speed);
                ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
                ImGui::LabelText("##PERCENT_Label", "%.1f%%", (float)clock_speed / 16777216.0f * 100.0f);

                ImGui::EndMenuBar();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();

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