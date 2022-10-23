#include "Frontend.hpp"
#include "common/Version.hpp"
#include "common/Log.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>


EmuThread::EmuThread(emu::GBA &core) : m_core(core) {
    m_cycle_diff = 0;

    m_core.debugger.onBreak([this] () {
        m_running.store(false);
        m_clock_speed.store(0);
    });
}

EmuThread::~EmuThread() {
    if(m_thread.joinable()) {
        m_thread.join();
    }
}

void EmuThread::start() {
    //Don't do anything if already running
    if(m_thread.joinable()) {
        if(!m_running.load()) {
            m_thread.join();
        } else {
            return;
        }
    }

    m_clock_speed.store(0);
    m_clock_start = m_core.scheduler.getCurrentTimestamp();
    m_start = std::chrono::steady_clock::now();
    m_running.store(true);
    m_cycle_diff = 0;

    m_thread = std::thread([this]() {
        while(m_running.load()) {
            if(std::chrono::steady_clock::now() >= (m_start + std::chrono::seconds(1))) {
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - m_start);
                m_clock_speed.store((m_core.scheduler.getCurrentTimestamp() - m_clock_start) / ((float)duration.count() / 1000000.0f));
                m_clock_start = m_core.scheduler.getCurrentTimestamp();
                m_start = std::chrono::steady_clock::now();
            }

            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, [&]() { return m_run; });
            m_run = false;
            lock.unlock();

            u32 cycles_left = (16777216 / 64) + m_cycle_diff;
            // auto start = std::chrono::steady_clock::now();
            u32 actual = m_core.run(cycles_left);
            // LOG_INFO("Ran {} cycles in {}ms", cycles_left, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());
            m_cycle_diff = (s32)cycles_left - actual;
            // m_core.run(16777216 / 64);
        }
    });
}

void EmuThread::stop() {
    //Don't do anything if not running
    if(!m_thread.joinable()) {
        return;
    }

    m_running.store(false);
    m_thread.join();
    m_clock_speed.store(0);
}

auto EmuThread::running() const -> bool {
    return m_running.load();
}

void EmuThread::runNext() {
    m_mutex.lock();
    m_run = true;
    m_cv.notify_one();
    m_mutex.unlock();
}

auto EmuThread::getClockSpeed() const -> u64 {
    return m_clock_speed.load();
}

Frontend::Frontend(GLFWwindow *window) : m_input_device(window), m_core(m_video_device, m_input_device, m_audio_device), m_debug_ui(m_core),
    m_emu_thread(m_core) {
    glfwGetWindowSize(window, &m_width, &m_height);
    m_window = window;
    m_show_cpu_debug = false;
    m_show_disasm_debug = false;
    m_show_bkpt_debug = false;
    m_show_scheduler_debug = false;
    m_show_about = false;
    m_show_pak_info = false;
    m_user_data = {this, &m_core};
}

void Frontend::init() {
    //Set Window title to the title in the ROM's header
    glfwSetWindowTitle(m_window, fmt::format("gba  [{}] - {}", common::GIT_DESC, m_core.getGamePak().getTitle()).c_str());
    
    //Setup callbacks
    glfwSetWindowUserPointer(m_window, &m_user_data);
    glfwSetWindowSizeCallback(m_window, windowSizeCallback);

    //Spawn emulator thread
    m_emu_thread.start();
}

void Frontend::shutdown() {
    m_emu_thread.stop();
}

void Frontend::loadROM(std::vector<u8> &&rom) {
    m_core.loadROM(std::move(rom));
}

void Frontend::loadBIOS(const std::vector<u8> &bios) {
    m_core.loadBIOS(bios);
}

void Frontend::loadSave(const std::string &filename) {
    m_core.loadSave(filename);
}

void Frontend::writeSave(const std::string &filename) {
    m_core.writeSave(filename);
}

void Frontend::drawInterface() {
    beginFrame();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::BeginMainMenuBar();

    if(ImGui::BeginMenu("Emulation")) {
        if(ImGui::MenuItem("Start")) {
            m_emu_thread.start();
        }
        if(ImGui::MenuItem("Stop")) {
            m_emu_thread.stop();
        }
        if(ImGui::MenuItem("Reset")) {
            m_emu_thread.stop();
            m_core.reset();
            m_emu_thread.start();
        }

        if(!m_emu_thread.running() && ImGui::MenuItem("Next Frame", "N")) {
            m_core.run(280896); //1 frame of cycles
        }

        ImGui::EndMenu();
    }

    static bool last_key, last_key_2;

    if(!m_emu_thread.running() && !last_key && glfwGetKey(m_window, GLFW_KEY_N) == GLFW_PRESS) {
        m_core.run(280896);
    }

    last_key = glfwGetKey(m_window, GLFW_KEY_N) == GLFW_PRESS;

    if(m_emu_thread.running() && !last_key && glfwGetKey(m_window, GLFW_KEY_P) == GLFW_PRESS) {
        m_emu_thread.stop();
    }

    last_key_2 = glfwGetKey(m_window, GLFW_KEY_P) == GLFW_PRESS;

    if(ImGui::BeginMenu("Debug")) {
        if(ImGui::MenuItem("CPU")) {
            m_show_cpu_debug = true;
        }
        if(ImGui::MenuItem("Disassembly")) {
            m_show_disasm_debug = true;
        }
        if(ImGui::MenuItem("Breakpoints")) {
            m_show_bkpt_debug = true;
        }
    //     if(ImGui::MenuItem("Memory Viewer")) {
    //         show_mem_debug = true;
    //     }
    //     if(ImGui::MenuItem("Framebuffer")) {
    //         show_vram_debug = true;
    //     }
        if(ImGui::MenuItem("Scheduler")) {
            m_show_scheduler_debug = true;
        }
        if(ImGui::MenuItem("Pak Info")) {
            m_show_pak_info = true;
        }

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Help")) {
        if(ImGui::MenuItem("About")) {
            m_show_about = true;
        }

        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();

    //Status Bar
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ImGui::GetStyle().WindowPadding.x, 0));
    if(ImGui::BeginViewportSideBar("##StatusBar", ImGui::GetMainViewport(), ImGuiDir_Down, ImGui::GetFrameHeight(), ImGuiWindowFlags_MenuBar)) {
        if(ImGui::BeginMenuBar()) {
            ImGui::Text("FPS: %4.1f", ImGui::GetIO().Framerate);
            ImGui::Dummy(ImVec2(5, 0));
            ImGui::Text("Clock Speed: %lluhz", m_emu_thread.getClockSpeed());
            ImGui::Dummy(ImVec2(5, 0));
            ImGui::Text("%.1f%%", (float)m_emu_thread.getClockSpeed() / 16777216.0f * 100.0f);

            ImGui::EndMenuBar();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();

    ImGui::SetNextWindowSize(ImVec2(m_width, m_height - ImGui::GetFrameHeight() * 2));
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if(ImGui::Begin("##GBA Screen", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration)) {
        m_debug_ui.drawScreen();
    }
    ImGui::End();
    ImGui::PopStyleVar(2);

    bool running = m_emu_thread.running();

    if(m_show_bkpt_debug) {
        if(ImGui::Begin("Breakpoints", &m_show_bkpt_debug)) m_debug_ui.drawBreakpoints(running);
        ImGui::End();
    }

    if(m_show_cpu_debug) {
        if(ImGui::Begin("CPU Debugger", &m_show_cpu_debug)) m_debug_ui.drawCPUDebugger(running);
        ImGui::End();
    }

    if(m_show_disasm_debug) {
        if(ImGui::Begin("Disassembly", &m_show_disasm_debug)) m_debug_ui.drawDisassembly();
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

    if(m_show_scheduler_debug) {
        if(ImGui::Begin("Scheduler", &m_show_scheduler_debug)) m_debug_ui.drawSchedulerViewer();
        ImGui::End();
    }

    if(m_show_pak_info) {
        if(ImGui::Begin("Pak Info", &m_show_pak_info)) {
            emu::GamePak &pak = m_core.getGamePak();
            const emu::GamePakHeader &header = pak.getHeader();
            ImGui::Text("Size: %u bytes", pak.size());
            ImGui::Text("Internal Title: %s", pak.getTitle().c_str());
            ImGui::Text("Game Code: %c%c%c%c", header.game_code[0], header.game_code[1], header.game_code[2], header.game_code[3]);
            ImGui::Text("Maker Code: %c%c", header.maker_code[0], header.maker_code[1]);
            ImGui::Text("Version: %i", header.version);
            ImGui::Text("Device Type: %i", header.device_type);
            ImGui::Text("Main Unit Code: %i", header.unit_code);
            ImGui::Text("Checksum: %i", header.checksum);
        }

        ImGui::End();
    }

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

    if(ImGui::Begin("Performance")) {
        float values_1[50];
        float values_2[50];

        for(size_t i = 0; i < m_frame_times.size(); i++) {
            values_1[i] = m_frame_times[i];
        }

        for(size_t i = 0; i < m_video_device.getFrameTimes().size(); i++) {
            values_2[i] = m_video_device.getFrameTimes()[i];
        }

        ImGui::PlotLines("Host Frame Times", values_1, 50, 0, nullptr, 0.0f, 36.0f, ImVec2(0.0f, ImGui::GetContentRegionAvail().y / 2.0f));
        ImGui::PlotLines("Guest Frame Times", values_2, 50, 0, nullptr, 0.0f, 36.0f, ImVec2(0.0f, ImGui::GetContentRegionAvail().y));
    }
    ImGui::End();

    if(ImGui::Begin("Audio Buffer Health")) {
        float values_1[512];

        for(size_t i = 0; i < m_sample_buffer_health.size(); i++) {
            values_1[i] = m_sample_buffer_health[i];
        }

        ImGui::PlotLines("Audio Buffer Size", values_1, 512, 0, nullptr, -1.0f, 1.0f, ImVec2(0.0f, ImGui::GetContentRegionAvail().y / 2.0f));
    }
    ImGui::End();

    endFrame();
}

void Frontend::audio_sync(ma_device *device, void *output, const void *input, ma_uint32 frame_count) {
    Frontend *frontend = reinterpret_cast<Frontend*>(device->pUserData);
    frontend->m_emu_thread.runNext();

    MAAudioDevice &audio_device = frontend->m_audio_device;

    float *f_output = reinterpret_cast<float*>(output);

    // frontend->m_sample_buffer_health.push_back(audio_device.m_samples.size());
    // if(frontend->m_sample_buffer_health.size() > 512) {
    //     frontend->m_sample_buffer_health.pop_front();
    // }

    if(audio_device.m_samples.size() < 512) {
        LOG_ERROR("Not enough samples for audio callback");
        return;
    }

    //Resample 512 samples to 750
    float samples[512];

    for(int i = 0; i < 512; i++) {
        samples[i] = audio_device.m_samples.peek();
        audio_device.m_samples.pop();
        frontend->m_sample_buffer_health.push_back(samples[i]);
        if(frontend->m_sample_buffer_health.size() > 512) {
            frontend->m_sample_buffer_health.pop_front();
        }
    }


    for(u32 i = 0; i < frame_count; i++) {
        float t = (float)i / (float)frame_count;
        f_output[i * 2] = samples[(size_t)(t * 512)];
        f_output[i * 2 + 1] = samples[(size_t)(t * 512)];
    }
}

void Frontend::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    m_input_device.update();
}

void Frontend::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    //Frame times
    auto now = std::chrono::steady_clock::now();
    auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(now - m_start);
    m_start = now;

    m_frame_times.push_back((float)frame_time.count() / 1000.0f);
    if(m_frame_times.size() > 50) {
        m_frame_times.pop_front();
    }
}

void Frontend::windowSizeCallback(GLFWwindow *window, int width, int height) {
    CallbackUserData *user_data = reinterpret_cast<CallbackUserData*>(glfwGetWindowUserPointer(window));

    if(user_data != nullptr) {
        Frontend *instance = reinterpret_cast<Frontend*>(user_data->frontend);
        instance->m_width = width;
        instance->m_height = height;

        //Redraw gui
        instance->drawInterface();
        glfwSwapBuffers(window);
    }
}