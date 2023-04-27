#include "Frontend.hpp"
#include "common/Version.hpp"
#include "common/Log.hpp"
#include "common/File.hpp"
#include "fonts/RubikRegular.hpp"
#include "fonts/NotoSansMonoMedium.hpp"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <filesystem>


Frontend::Frontend(GLFWwindow *window) : m_input_device(window),
m_audio_device([this](bool new_samples, float *samples, size_t buffer_size) { audioCallback(new_samples, samples, buffer_size); }),
m_core(std::make_shared<emu::GBA>(m_video_device, m_input_device, m_audio_device)), m_emu_thread(m_core) {
    LOG_DEBUG("Initializing Frontend...");

    //Set window stuff
    m_window = window;
    glfwSetWindowTitle(m_window, fmt::format("gba  [{}]", common::GIT_DESC).c_str());
    glfwSwapInterval(1);

    //Setup callbacks and retrieve window dimensions
    glfwGetWindowSize(m_window, &m_width, &m_height);
    glfwSetWindowUserPointer(m_window, &m_user_data);
    glfwSetWindowSizeCallback(m_window, windowSizeCallback);

    m_user_data = {this, m_core};
    rom_loaded = false;

    m_windows.push_back(&m_about_dialog);
    m_windows.push_back(&m_file_dialog);

    std::memset(m_audio_samples_l, 0, sizeof(m_audio_samples_l));
    std::memset(m_audio_samples_r, 0, sizeof(m_audio_samples_r));
}

void Frontend::init() {
    //Initialize Dear ImGui here so callbacks are installed correctly
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    io.Fonts->AddFontFromMemoryCompressedTTF(rubik_regular_compressed_data, rubik_regular_compressed_size, 15);
    io.Fonts->AddFontFromMemoryCompressedTTF(noto_sans_mono_medium_compressed_data, noto_sans_mono_medium_compressed_size, 15);
    io.Fonts->Build();
 
    refreshScreenDimensions();
    refreshGameList();
}

void Frontend::shutdown() {
    m_emu_thread.stop();
    m_audio_device.stop();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Frontend::mainloop() {
    while(!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        drawInterface();
        glfwSwapBuffers(m_window);
    }
}

void Frontend::startEmulation() {
    m_emu_thread.start();
    m_audio_device.start();
}

void Frontend::stopEmulation() {
    m_audio_device.stop();
    m_emu_thread.stop();
}

void Frontend::resetEmulation() {
    bool running = m_emu_thread.running();

    if(running) {
        stopEmulation();
    }

    m_core->reset();
    m_video_device.reset();

    if(running) {
        startEmulation();
    }
}

void Frontend::resetAndLoad(const std::string &path) {
    stopEmulation();
    
    if(loadROM(path)) {
        m_core->reset();
        m_video_device.reset();
    }
    startEmulation();
}

auto Frontend::loadROM(const std::string &path) -> bool {
    if(m_core->bus.pak.loadFile(path)) {
        m_core->cpu.flushPipeline();
        rom_loaded = true;

        //Set Window title to the title in the ROM's header
        glfwSetWindowTitle(m_window, fmt::format("gba  [{}] - {}", common::GIT_DESC, m_core->bus.pak.getTitle()).c_str());

        return true;
    } else {
        LOG_ERROR("Failed to load file '{}'!", path);
        return false;
    }
}

void Frontend::loadBIOS(const std::vector<u8> &bios) {
    m_core->loadBIOS(bios);
}

void Frontend::refreshGameList() {
    std::filesystem::directory_iterator iter(std::filesystem::current_path());
    game_list.clear();

    for(const auto &entry : iter) {
        if(entry.is_regular_file() && entry.path().extension() == ".gba") {
            game_list.push_back(entry.path().filename().string());
        }
    }
}

void Frontend::refreshScreenDimensions() {
    ImVec2 available = ImVec2(m_width, m_height - ImGui::GetFrameHeight());
    float gba_aspect_ratio = 240.0f / 160.0f;

    if(available.x / available.y > gba_aspect_ratio) {
        screen_width = available.y * gba_aspect_ratio;
        screen_height = available.y;
    } else {
        screen_width = available.x;
        screen_height = available.x / gba_aspect_ratio;
    }
}

void Frontend::drawInterface() {
    beginFrame();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);

    if(ImGui::BeginMainMenuBar()) {
        if(ImGui::BeginMenu("File")) {
            if(ImGui::MenuItem("Load ROM")) {
                m_file_dialog.open();
            }
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Emulation")) {
            if(ImGui::MenuItem("Pause", nullptr, rom_loaded && !m_emu_thread.running(), rom_loaded)) {
                if(m_emu_thread.running()) {
                    stopEmulation();
                } else {
                    startEmulation();
                }
            }

            if(ImGui::MenuItem("Next frame", nullptr, nullptr, rom_loaded && !m_emu_thread.running())) {
                m_core->run(280896);
            }

            if(ImGui::MenuItem("Fast forward", nullptr, m_emu_thread.fastforwarding(), rom_loaded)) {
                m_emu_thread.setFastforward(!m_emu_thread.fastforwarding());
            }

            if(ImGui::MenuItem("Reset", nullptr, nullptr, rom_loaded)) {
                resetEmulation();
            }

            if(ImGui::MenuItem("Stop", nullptr, nullptr, rom_loaded)) {
                stopEmulation();
                m_core->bus.pak.unload();
                m_emu_thread.setFastforward(false);
                m_video_device.clear(0);
                m_audio_device.clear();
                rom_loaded = false;
            }
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("View")) {
            ImGui::EndMenu();
        }
        
        if(ImGui::BeginMenu("Help")) {
            if(ImGui::MenuItem("About")) {
                m_about_dialog.open();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if(rom_loaded) {
        ImGui::SetNextWindowSize(ImVec2(m_width, m_height - ImGui::GetFrameHeight()));
        ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if(ImGui::Begin("##GBA Screen", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration)) {
            ImVec2 available = ImGui::GetContentRegionAvail();
            
            ImGui::SetCursorPos(ImVec2((available.x - screen_width) * 0.5f, (available.y - screen_height) * 0.5f));
            ImGui::Image((void*)(intptr_t)m_video_device.getTextureID(), ImVec2(screen_width, screen_height));
        }
        ImGui::End();
        ImGui::PopStyleVar();
    } else {
        ImVec2 padding = ImGui::GetStyle().WindowPadding;
        
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGui::SetNextWindowSize(ImVec2(m_width, m_height - ImGui::GetFrameHeight()));
        ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if(ImGui::Begin("Game List", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration)) {
            ImGui::SetCursorPos(padding);

            if(ImGui::Button("Refresh")) {
                refreshGameList();
            }

            ImGui::SameLine();
            ImGui::Text("Size: %zu", game_list.size());
            ImGui::Separator();

            ImGui::BeginChild("##GameList_List");
            ImGuiTableFlags table_flags = ImGuiTableFlags_RowBg;
            ImGui::BeginTable("##GameList_Table", 2, table_flags);
            
            ImGuiListClipper clipper(game_list.size());            
            while(clipper.Step()) {
                for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);

                    ImVec2 item_size = ImVec2(0, ImGui::GetTextLineHeightWithSpacing());
                    if(ImGui::Selectable(fmt::format(" {:<30}", game_list[i]).c_str(), false, ImGuiSelectableFlags_SpanAllColumns, item_size)) {
                        resetAndLoad(game_list[i]);
                    }

                    ImGui::TableNextColumn();

                    float size = std::filesystem::file_size(game_list[i]) / 1_KiB;
                    ImGui::Text("%.1f KiB", size);
                }
            }

            ImGui::EndTable();
            ImGui::EndChild();
            
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }


    for(auto window : m_windows) {
        if(window->active) {
            window->draw(*this);
        }
    }

    ImGui::PopStyleVar(2);

    endFrame();
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

    m_frame_times.push((float)frame_time.count() / 1000.0f);

    //Calculate framerate as a moving average of the last 100 frames
    m_average_fps = 0;
    size_t host_size = 0;
    for(size_t i = 0; i < 100; i++) {
        if(m_frame_times.peek(i) != 0) {
            m_average_fps += m_frame_times.peek(i);
            host_size++;
        }
    }
    // (1 / (average / 100)) * 1000
    m_average_fps = (1000.0f * host_size) / m_average_fps;

    //Calculate framerate as a moving average of the last 100 frames
    const auto &gba_frame_times = m_video_device.getFrameTimes();
    m_gba_fps = 0;
    for(size_t i = 0; i < gba_frame_times.size(); i++) {
        if(gba_frame_times.peek(i) != 0) {
            m_gba_fps += gba_frame_times.peek(i);
        }
    }
    m_gba_fps = (1000.0f * gba_frame_times.size()) / m_gba_fps;
    // LOG_INFO("Speed: {}%", (m_gba_fps / 59.7275f) * 100.0f);
}

void Frontend::audioCallback(bool new_samples, float *samples, size_t buffer_size) {
    m_emu_thread.sendCommand(RUN);
    m_audio_buffer_mutex.lock();
    m_audio_buffer_size.push(buffer_size);
    m_audio_buffer_mutex.unlock();

    if(!new_samples) {
        return;
    }

    m_audio_buffer_mutex.lock();
    std::memcpy(m_audio_samples_l, samples, 750 * sizeof(float));
    std::memcpy(m_audio_samples_r, samples + 750, 750 * sizeof(float));
    m_audio_buffer_mutex.unlock();
}

void Frontend::windowSizeCallback(GLFWwindow *window, int width, int height) {
    CallbackUserData *user_data = reinterpret_cast<CallbackUserData*>(glfwGetWindowUserPointer(window));

    if(user_data != nullptr) {
        Frontend *instance = reinterpret_cast<Frontend*>(user_data->frontend);
        instance->m_width = width;
        instance->m_height = height;

        //Redraw gui
        instance->refreshScreenDimensions();
        instance->drawInterface();
        glfwSwapBuffers(window);
    }
}