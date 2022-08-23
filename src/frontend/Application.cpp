#include "Application.hpp"
#include "common/Version.hpp"
#include "common/Log.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>


Application::Application(GLFWwindow *window) : m_input_device(window), m_core(m_video_device, m_input_device), m_debug_ui(m_core) {
    m_window = window;
    glfwGetWindowSize(window, &m_width, &m_height);

    m_user_data = {this, &m_core};

    m_show_cpu_debug = false;
    m_show_about = false;
}

void Application::init() {
    //Set Window title to the title in the ROM's header
    char game_title[13];
    std::memcpy(game_title, m_core.getGamePak().getHeader().title, 12);
    game_title[12] = '\0';
    glfwSetWindowTitle(m_window, fmt::format("gba  [{}] - {}", common::GIT_DESC, game_title).c_str());
    
    //Setup callbacks
    glfwSetWindowUserPointer(m_window, &m_user_data);
    glfwSetWindowSizeCallback(m_window, windowSizeCallback);

    //Spawn emulator thread
    m_start = std::chrono::steady_clock::now();
    m_clock_start = m_core.getCurrentTimestamp();
    m_clock_speed.store(0);
    m_stop.store(false);
    m_core.getDebugger().setRunning(true);
    m_emu_thread = std::thread([this] {
        while(true) {
            if(std::chrono::steady_clock::now() >= (m_start + std::chrono::seconds(1))) {
                m_start = std::chrono::steady_clock::now();
                m_clock_speed.store(m_core.getCurrentTimestamp() - m_clock_start);
                m_clock_start = m_core.getCurrentTimestamp();
            }

            if(m_stop.load()) {
                break;
            }

            if(m_should_run.exchange(false) && !m_core.run(167772)) {
            // if(!m_core.run(167772)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    });
}

void Application::shutdown() {
    m_stop.store(true);
    m_emu_thread.join();
}

void Application::loadROM(std::vector<u8> &&rom) {
    m_core.loadROM(std::move(rom));
}

void Application::loadBIOS(const std::vector<u8> &bios) {
    m_core.loadBIOS(bios);
}

void Application::drawInterface() {
    beginFrame();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::BeginMainMenuBar();
    if(ImGui::BeginMenu("Debug")) {
    //     if(ImGui::MenuItem("Breakpoints")) {
    //         show_bkpts_debug = true;
    //     }
        if(ImGui::MenuItem("CPU")) {
            m_show_cpu_debug = true;
        }
    //     if(ImGui::MenuItem("Memory Viewer")) {
    //         show_mem_debug = true;
    //     }
    //     if(ImGui::MenuItem("Framebuffer")) {
    //         show_vram_debug = true;
    //     }
    //     if(ImGui::MenuItem("Scheduler")) {
    //         show_scheduler_debug = true;
    //     }
    //     if(ImGui::MenuItem("Cart Info")) {
    //         show_pak_info = true;
    //     }

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Help")) {
        if(ImGui::MenuItem("About")) {
            m_show_about = true;
        }

        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();

    ImGui::SetNextWindowSize(ImVec2(m_width, m_height - ImGui::GetFrameHeight() * 2));
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if(ImGui::Begin("##GBA Screen", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration)) {
        m_debug_ui.drawScreen();
    }
    ImGui::End();
    ImGui::PopStyleVar(2);


    // if(show_bkpts_debug) {
    //     if(ImGui::Begin("Breakpoints", &show_bkpts_debug)) debug_ui.drawBreakpoints();
    //     ImGui::End();
    // }

    if(m_show_cpu_debug) {
        if(ImGui::Begin("CPU Debugger", &m_show_cpu_debug)) m_debug_ui.drawCPUDebugger();
        ImGui::End();
    }

    // if(show_mem_debug) {
    //     if(ImGui::Begin("Memory Viewer", &show_mem_debug)) debug_ui.drawMemoryViewer();
    //     ImGui::End();
    // }

    // if(show_vram_debug) {
    //     if(ImGui::Begin("Framebuffer", &show_vram_debug)) debug_ui.drawPPUState();
    //     ImGui::End();
    // }

    // if(show_scheduler_debug) {
    //     if(ImGui::Begin("Scheduler", &show_scheduler_debug)) debug_ui.drawSchedulerViewer();
    //     ImGui::End();
    // }

    // if(show_pak_info) {
    //     if(ImGui::Begin("Pak Info", &show_pak_info)) {
    //         emu::GamePak &pak = gba.getGamePak();
    //         emu::GamePakHeader &header = pak.getHeader();
    //         ImGui::Text("Size: %u bytes", pak.size());
    //         ImGui::Text("Name: %s", argv[1]);
    //         ImGui::Text("Internal Title: %s", game_title);
    //         ImGui::Text("Game Code: %c%c%c%c", header.game_code[0], header.game_code[1], header.game_code[2], header.game_code[3]);
    //         ImGui::Text("Maker Code: %c%c", header.maker_code[0], header.maker_code[1]);
    //         ImGui::Text("Version: %i", header.version);
    //         ImGui::Text("Device Type: %i", header.device_type);
    //         ImGui::Text("Main Unit Code: %i", header.unit_code);
    //         ImGui::Text("Checksum: %i", header.checksum);
    //     }

    //     ImGui::End();
    // }

    if(m_show_about) {
        if(ImGui::Begin("About", &m_show_about)) {
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
            ImGui::SetNextItemWidth(ImGui::CalcTextSize("FPS: 00.0 ").x);
            ImGui::LabelText("##FPS_Label", "FPS: %4.1f", ImGui::GetIO().Framerate);
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SetNextItemWidth(ImGui::CalcTextSize("Clock Speed: 00000000hz ").x);
            ImGui::LabelText("##CLOCKSPEED_Label", "Clock Speed: %8lluhz", m_clock_speed.load());
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::LabelText("##PERCENT_Label", "%.1f%%", (float)m_clock_speed.load() / 16777216.0f * 100.0f);

            ImGui::EndMenuBar();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();

    endFrame();
}

void Application::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Application::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Application::windowSizeCallback(GLFWwindow *window, int width, int height) {
    CallbackUserData *user_data = reinterpret_cast<CallbackUserData*>(glfwGetWindowUserPointer(window));

    if(user_data != nullptr) {
        Application *instance = reinterpret_cast<Application*>(user_data->application);

        instance->m_width = width;
        instance->m_height = height;

        //Redraw gui
        instance->drawInterface();
        glfwSwapBuffers(window);
    }
}