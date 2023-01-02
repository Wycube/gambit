#include "Frontend.hpp"
#include "common/Version.hpp"
#include "common/Log.hpp"
#include "ui/Common.hpp"
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

            // auto start = std::chrono::steady_clock::now();
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, [&]() { return m_run; });
            m_run = false;
            lock.unlock();
            // LOG_INFO("Waited for {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());

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

    m_running.
    store(false);
    runNext();
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

Frontend::Frontend(GLFWwindow *window) : m_input_device(window), m_audio_device(audio_sync, this),
m_core(m_video_device, m_input_device, m_audio_device), 
m_debug_ui(m_core), m_emu_thread(m_core) {
    LOG_DEBUG("Initializing Frontend...");

    //Set window stuff
    m_window = window;
    glfwSetWindowTitle(m_window, fmt::format("gba  [{}]", common::GIT_DESC).c_str());
    glfwSwapInterval(1);

    //Setup callbacks and retrieve window dimensions
    glfwGetWindowSize(m_window, &m_width, &m_height);
    glfwSetWindowUserPointer(m_window, &m_user_data);
    glfwSetWindowSizeCallback(m_window, windowSizeCallback);

    m_show_status_bar = true;
    m_show_cpu_debug = false;
    m_show_disasm_debug = false;
    m_show_bkpt_debug = false;
    m_show_scheduler_debug = false;
    // m_show_about = false;
    m_show_pak_info = false;
    m_show_settings = true;
    m_user_data = {this, &m_core};

    // m_frame_times_start = 0;
    // m_audio_buffer_size_start = 0;
    std::memset(m_audio_samples_l, 0, sizeof(m_audio_samples_l));
    std::memset(m_audio_samples_r, 0, sizeof(m_audio_samples_r));
    // std::memset(m_audio_buffer_size, 0, sizeof(m_audio_buffer_size));
    // std::memset(m_frame_times, 0, sizeof(m_frame_times));
}

void Frontend::init() {
    //Spawn emulator thread
    m_emu_thread.start();

    m_audio_device.start();
}

void Frontend::shutdown() {
    m_emu_thread.stop();
}

void Frontend::mainloop() {
    while(!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        drawInterface();
        glfwSwapBuffers(m_window);
    }
}

void Frontend::loadROM(std::vector<u8> &&rom) {
    m_core.loadROM(std::move(rom));

    //Set Window title to the title in the ROM's header
    glfwSetWindowTitle(m_window, fmt::format("gba  [{}] - {}", common::GIT_DESC, m_core.getGamePak().getTitle()).c_str());
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

    if(ImGui::BeginMenu("File")) {
        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Edit")) {
        ImGui::MenuItem("Settings", nullptr, &m_show_settings);
        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Emulation")) {
        if(ImGui::MenuItem("Start")) {
            m_emu_thread.start();
            m_audio_device.start();
        }
        if(ImGui::MenuItem("Stop")) {
            m_audio_device.stop();
            m_emu_thread.stop();
        }
        if(ImGui::MenuItem("Reset")) {
            m_audio_device.stop();
            m_emu_thread.stop();
            m_core.reset();
            m_emu_thread.start();
            m_audio_device.start();
        }

        if(!m_emu_thread.running() && ImGui::MenuItem("Next Frame", "N")) {
            m_core.run(280896); //1 frame of cycles
        }

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("View")) {
        if(ImGui::MenuItem("Status Bar", nullptr, m_show_status_bar)) {
            m_show_status_bar = !m_show_status_bar;
        }

        if(ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("Metrics", nullptr, m_metrics_window.getActive());
            
            if(ImGui::MenuItem("CPU")) {
                m_show_cpu_debug = true;
            }
            if(ImGui::MenuItem("Disassembly")) {
                m_show_disasm_debug = true;
            }
            if(ImGui::MenuItem("Breakpoints")) {
                m_show_bkpt_debug = true;
            }
            // if(ImGui::MenuItem("Memory Viewer")) {
            //     show_mem_debug = true;
            // }
            // if(ImGui::MenuItem("Framebuffer")) {
            //     show_vram_debug = true;
            // }
            if(ImGui::MenuItem("Scheduler")) {
                m_show_scheduler_debug = true;
            }
            if(ImGui::MenuItem("Pak Info")) {
                m_show_pak_info = true;
            }

            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Help")) {
        ImGui::MenuItem("About", nullptr, m_about_window.getActive());

        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();

    //Status Bar
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ImGui::GetStyle().WindowPadding.x, 0));
    if(m_show_status_bar && ImGui::BeginViewportSideBar("##StatusBar", ImGui::GetMainViewport(), ImGuiDir_Down, ImGui::GetFrameHeight(), ImGuiWindowFlags_MenuBar)) {
        if(ImGui::BeginMenuBar()) {
            ImGui::Text("FPS: %4.1f", m_average_fps);
            ImGui::Dummy(ImVec2(5, 0));
            ImGui::Text("FPS: %4.1f", m_gba_fps);
            ImGui::Dummy(ImVec2(5, 0));
            ImGui::Text("Clock Speed: %zuhz", m_emu_thread.getClockSpeed());
            ImGui::Dummy(ImVec2(5, 0));
            ImGui::Text("%.1f%%", (float)m_emu_thread.getClockSpeed() / 16777216.0f * 100.0f);

            ImGui::EndMenuBar();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();

    ImGui::SetNextWindowSize(ImVec2(m_width, m_height - ImGui::GetFrameHeight() * (m_show_status_bar ? 2 : 1)));
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

    // if(ImGui::Begin("Input")) {
    //     // const char *gamepads[GLFW_JOYSTICK_LAST + 1];
    //     std::string devices;
    //     devices = "Keyboard";
    //     for(int i = 0; i < GLFW_JOYSTICK_LAST; i++) {
    //         if(glfwJoystickPresent(i) == GLFW_TRUE) {
    //             // gamepads[i] = glfwGetJoystickName(i);
    //             devices += '\0';
    //             devices += glfwGetJoystickName(i);
    //         }
    //     }

    //     static int current;

    //     ImGui::Combo("Input Devices", &current, devices.c_str());
    // }
    // ImGui::End();


    // ---------- Settings (Under Construction) ----------


    // if(m_show_settings) {
    //     if(ImGui::Begin("Settings", &m_show_settings)) {
    //         //Tabs
    //         ImGui::BeginGroup();

    //         static int current_item;
    //         static const char *categories[5] = {"General", "UI", "Emulation", "Graphics", "Audio"};

    //         ImGui::SetNextItemWidth(ImGui::CalcTextSize("Emulation ").x);
    //         ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    //         ImGui::ListBox("##Categories", &current_item, categories, 5);
    //         ImGui::PopStyleColor();
    //         ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y));
    //         ImGui::EndGroup();

    //         ImGui::SameLine();
    //         ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

    //         ImGui::SameLine();
    //         ImGui::BeginGroup();
    //         ui::BeginGroupPanel(categories[current_item]);

    //         ImGui::Text("Something");
    //         ImGui::SameLine();
    //         static int awdmalkmdwlka;
    //         ImGui::SetNextItemWidth(ImGui::CalcTextSize("Software Renderer ").x);
    //         ImGui::Combo("##SomethingCombo", &awdmalkmdwlka, "Default\0Another One\0High Resolution\0Software Renderer\0");

    //         ui::EndGroupPanel();
    //         ImGui::EndGroup();
    //     }
    //     ImGui::End();
    // }


    // ---------- End of Settings ----------


    // if(m_show_about) {
    //     if(ImGui::Begin("About", &m_show_about)) {
    //         ImGui::Text("Game Boy Advance Emulator,");
    //         ImGui::Text("Copyright (c) 2021-2022 Wycube");
    //         ImGui::Separator();
    //         ImGui::Text("Version: %s", common::GIT_DESC);
    //         ImGui::Text("Commit: %s", common::GIT_COMMIT);
    //         ImGui::Text("Branch: %s", common::GIT_BRANCH);
    //     }

    //     ImGui::End();
    // }
    m_about_window.update();

    // if(ImGui::Begin("Performance")) {
    //     float values_1[100];
    //     float values_2[100];

    //     std::memcpy(values_1, &m_frame_times[m_frame_times_start], (100 - m_frame_times_start) * sizeof(float));
    //     std::memcpy(&values_1[100 - m_frame_times_start], m_frame_times, m_frame_times_start * sizeof(float));

    //     m_video_device.getFrameTimes().copy(values_2);

    //     ImGui::PlotLines("##Host_Frames", values_1, 100, 0, "Host Frame Times", 33.3f, 0.0f, ImVec2(0.0f, ImGui::GetContentRegionAvail().y / 2.0f));
    //     ImGui::PlotLines("##Guest_Frames", values_2, 100, 0, "Guest Frame Times", 33.3f, 0.0f, ImVec2(0.0f, ImGui::GetContentRegionAvail().y));
    // }
    // ImGui::End();

    float values_1[100];
    float values_2[100];
    // std::memcpy(values_1, &m_frame_times[m_frame_times_start], (100 - m_frame_times_start) * sizeof(float));
    // std::memcpy(&values_1[100 - m_frame_times_start], m_frame_times, m_frame_times_start * sizeof(float));
    m_frame_times.copy(values_1);
    m_video_device.getFrameTimes().copy(values_2);

    m_metrics_window.setHostTimes(values_1);
    m_metrics_window.setEmulatorTimes(values_2);
    m_metrics_window.update();

    if(ImGui::Begin("Audio Buffer Stats")) {
        float sizes[100];

        m_audio_buffer_mutex.lock();
        // std::memcpy(sizes, &m_audio_buffer_size[m_audio_buffer_size_start], (100 - m_audio_buffer_size_start) * sizeof(float));
        // std::memcpy(&sizes[100 - m_audio_buffer_size_start], m_audio_buffer_size, m_audio_buffer_size_start * sizeof(float));
        m_audio_buffer_size.copy(sizes);
        m_audio_buffer_mutex.unlock();

        ImGui::PlotLines("##Audio Buffer Size", sizes, 100, 0, "Audio Buffer Size", 0.0f, 2048.0f, ImVec2(0.0f, ImGui::GetContentRegionAvail().y / 2.0f));
        m_audio_buffer_mutex.lock();
        ImGui::PlotLines("##Output Audio Samples Left", m_audio_samples_l, 750, 0, "Output Audio Samples Left", -1.1f, 1.1f, ImVec2(0.0f, ImGui::GetContentRegionAvail().y / 2.0f));
        ImGui::PlotLines("##Output Audio Samples Right", m_audio_samples_r, 750, 0, "Output Audio Samples Right", -1.1f, 1.1f, ImVec2(0.0f, ImGui::GetContentRegionAvail().y));
        m_audio_buffer_mutex.unlock();
    }
    ImGui::End();

    endFrame();
}

void Frontend::audio_sync(ma_device *device, void *output, const void *input, ma_uint32 frame_count) {
    Frontend *frontend = reinterpret_cast<Frontend*>(device->pUserData);
    frontend->m_emu_thread.runNext();

    MAAudioDevice &audio_device = frontend->m_audio_device;

    float *f_output = reinterpret_cast<float*>(output);

    frontend->m_audio_buffer_mutex.lock();
    // frontend->m_audio_buffer_size[frontend->m_audio_buffer_size_start] = audio_device.samples_l.size();
    // frontend->m_audio_buffer_size_start = (frontend->m_audio_buffer_size_start + 1) % 100;
    frontend->m_audio_buffer_size.push(audio_device.samples_l.size());
    frontend->m_audio_buffer_mutex.unlock();

    if(audio_device.samples_l.size() < 1024) {
        // LOG_ERROR("Not enough samples for audio callback");
        return;
    }

    float samples[1500];
    audio_device.resample(samples, 750);
    frontend->m_audio_buffer_mutex.lock();
    std::memcpy(frontend->m_audio_samples_l, samples, 750 * sizeof(float));
    std::memcpy(frontend->m_audio_samples_r, samples + 750, 750 * sizeof(float));
    frontend->m_audio_buffer_mutex.unlock();


    for(size_t i = 0; i < frame_count; i++) {
        f_output[i * 2 + 0] = samples[i];
        f_output[i * 2 + 1] = samples[i + frame_count];
    }
}

void Frontend::beginFrame() {
    // m_start = std::chrono::steady_clock::now();
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

    // m_frame_times[m_frame_times_start] = (float)frame_time.count() / 1000.0f;
    // m_frame_times_start = (m_frame_times_start + 1) % 100;
    m_frame_times.push((float)frame_time.count() / 1000.0f);

    //Calculate framerate as a moving average of the last 100 frames
    float average = 0;
    for(size_t i = 0; i < 100; i++) {
        average += m_frame_times.peek(i) * (i + 1);
    }
    average /= (100.0f * 101.0f) / 2.0f;
    m_average_fps = 1.0f / average * 1000.0f;

    //Calculate framerate as a moving average of the last 100 frames
    average = 0;
    for(size_t i = 0; i < m_video_device.getFrameTimes().size(); i++) {
        average += m_video_device.getFrameTimes().peek(i) * (i + 1);
    }
    average /= (float)(m_video_device.getFrameTimes().size() * (m_video_device.getFrameTimes().size() + 1)) / 2.0f;
    m_gba_fps = 1.0f / average * 1000.0f;
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