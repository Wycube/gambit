#include "Frontend.hpp"
#include "common/Version.hpp"
#include "common/Log.hpp"
#include "common/File.hpp"
#include "fonts/RubikRegular.hpp"
#include "fonts/NotoSansMonoMedium.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <filesystem>


Frontend::Frontend(GLFWwindow *glfw_window) : window(glfw_window), input_device(glfw_window),
audio_device([this](bool new_samples, float *samples, size_t buffer_size) { audioCallback(new_samples, samples, buffer_size); }),
core(std::make_shared<emu::GBA>(video_device, input_device, audio_device)), emu_thread(core) {
    LOG_DEBUG("Initializing Frontend...");

    //Set window stuff
    glfwSetWindowTitle(window, fmt::format("gba  [{}]", common::GIT_DESC).c_str());
    glfwSwapInterval(1);

    //Setup callbacks and retrieve window position and size information
    fullscreen = false;
    glfwGetWindowPos(window, &last_pos_x, &last_pos_y);
    glfwGetWindowSize(window, &width, &height);
    last_width = width;
    last_height = height;
    glfwSetWindowUserPointer(window, &user_data);
    glfwSetWindowSizeCallback(window, windowSizeCallback);

    user_data = {this, core};
    rom_loaded = false;

    ui_windows.push_back(&about_dialog);
    ui_windows.push_back(&file_dialog);
    ui_windows.push_back(&metrics_window);
    ui_windows.push_back(&rom_info_window);
    ui_windows.push_back(&settings_window);

    std::memset(audio_samples_l, 0, sizeof(audio_samples_l));
    std::memset(audio_samples_r, 0, sizeof(audio_samples_r));
}

void Frontend::init() {
    //Initialize Dear ImGui here so callbacks are installed correctly
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    io.Fonts->AddFontFromMemoryCompressedTTF(rubik_regular_compressed_data, rubik_regular_compressed_size, 15);
    io.Fonts->AddFontFromMemoryCompressedTTF(noto_sans_mono_medium_compressed_data, noto_sans_mono_medium_compressed_size, 15);
    io.Fonts->Build();

    //ImGui::GetFrameHeight() will return a wrong value if done before any frame
    ImGui::NewFrame();
    ImGui::EndFrame();
 
    refreshScreenDimensions();
    refreshGameList();
}

void Frontend::shutdown() {
    emu_thread.stop();
    audio_device.stop();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Frontend::mainloop() {
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawInterface();
        glfwSwapBuffers(window);
    }
}

void Frontend::startEmulation() {
    emu_thread.start();
    audio_device.start();
}

void Frontend::stopEmulation() {
    audio_device.stop();
    emu_thread.stop();
}

void Frontend::resetEmulation() {
    bool running = emu_thread.running();

    if(running) {
        stopEmulation();
    }

    core->reset(settings.skip_bios, settings.enable_debugger);
    audio_buffer_sizes.clear();

    if(running) {
        startEmulation();
    }
}

void Frontend::resetAndLoad(const std::string &path) {
    stopEmulation();
    
    if(loadROM(path)) {
        core->reset(settings.skip_bios, settings.enable_debugger);
        audio_buffer_sizes.clear();
    }
    startEmulation();
}

auto Frontend::loadROM(const std::string &path) -> bool {
    if(core->bus.pak.loadFile(path)) {
        core->cpu.flushPipeline();
        rom_loaded = true;

        //Set Window title to the title in the ROM's header
        glfwSetWindowTitle(window, fmt::format("gba  [{}] - {}", common::GIT_DESC, core->bus.pak.getTitle()).c_str());

        return true;
    } else {
        LOG_ERROR("Failed to load file '{}'!", path);
        return false;
    }
}

void Frontend::loadBIOS(const std::vector<u8> &bios) {
    core->loadBIOS(bios);
}

auto Frontend::getWindow() -> GLFWwindow* {
    return window;
}

auto Frontend::getSettings() -> const Settings& {
    return settings;
}

void Frontend::setSettings(const Settings &new_settings) {
    bool status_bar_changed = settings.show_status_bar != new_settings.show_status_bar;
    bool rom_path_changed = settings.rom_path != new_settings.rom_path;
    bool input_source_changed = settings.input_source != new_settings.input_source;
    settings = new_settings;

    if(status_bar_changed) {
        refreshScreenDimensions();
    }

    if(rom_path_changed) {
        refreshGameList();
    }

    if(input_source_changed) {
        input_device.updateInputSource(settings.input_source);
    }
}

auto Frontend::getGamePakHeader() -> const emu::GamePakHeader& {
    return core->bus.pak.getHeader();
}

void Frontend::setToIntegerSize(int scale) {
    int new_width = 240 * scale;
    int new_height = 160 * scale;

    if(settings.show_menu_bar) {
        new_height += ImGui::GetFrameHeight();
    }

    if(settings.show_status_bar) {
        new_height += ImGui::GetFrameHeight();
    }

    just_changed_windowed_size = true;
    glfwSetWindowSize(window, new_width, new_height);
}

void Frontend::setFullscreen(bool fullscreen) {
    this->fullscreen = fullscreen;
    just_changed_windowed_size = true;

    if(fullscreen) {
        if(glfwGetWindowMonitor(window) != nullptr) {
            //Already in fullscreen
            return;
        }

        glfwGetWindowPos(window, &last_pos_x, &last_pos_y);
        glfwGetWindowSize(window, &last_width, &last_height);

        const GLFWvidmode *video_mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, video_mode->width, video_mode->height, GLFW_DONT_CARE);
    } else {
        if(glfwGetWindowMonitor(window) == nullptr) {
            //Already in windowed mode
            return;
        }

        glfwSetWindowMonitor(window, nullptr, last_pos_x, last_pos_y, last_width, last_height, GLFW_DONT_CARE);
    }
}

void Frontend::refreshScreenDimensions() {
    frame_height = 0;
    frame_height += settings.show_status_bar ? ImGui::GetFrameHeight() : 0;
    frame_height += settings.show_menu_bar ? ImGui::GetFrameHeight() : 0;
    const ImVec2 available = ImVec2(width, height - frame_height);
    const float gba_aspect_ratio = 240.0f / 160.0f;

    if(available.x / available.y > gba_aspect_ratio) {
        screen_width = available.y * gba_aspect_ratio;
        screen_height = available.y;
    } else {
        screen_width = available.x;
        screen_height = available.x / gba_aspect_ratio;
    }
}

void Frontend::refreshGameList() {
    if(settings.rom_path.empty()) {
        return;
    }

    std::filesystem::directory_iterator iter(settings.rom_path);
    game_list.clear();

    for(const auto &entry : iter) {
        if(entry.is_regular_file() && entry.path().extension() == ".gba") {
            game_list.push_back(entry.path().filename().string());
        }
    }
}

void Frontend::drawSizeMenuItems() {
    if(ImGui::MenuItem("Show Menu Bar", nullptr, &settings.show_menu_bar)) {
        refreshScreenDimensions();
    }

    if(ImGui::MenuItem("Show Status Bar", nullptr, &settings.show_status_bar)) {
        refreshScreenDimensions();
    }

    if(ImGui::MenuItem("Fullscreen", nullptr, fullscreen)) {
        setFullscreen(!fullscreen);
    }

    if(ImGui::BeginMenu("Integer Scale", !fullscreen)) {
        //Show integer values based on window resolution
        const GLFWvidmode *video_mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        int levels = std::fmin(video_mode->width / 240, video_mode->height / 160);

        for(int i = 1; i < levels; i++) {
            if(ImGui::MenuItem(fmt::format("{} ({}x{})", i, 240 * i, 160 * i).c_str())) {
                setToIntegerSize(i);
            }
        }

        ImGui::EndMenu();
    }
}

void Frontend::drawContextMenu() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{8, 8});

    if(ImGui::BeginPopupContextWindow()) {
        drawSizeMenuItems();

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
}

void Frontend::drawInterface() {
    beginFrame();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);

    if(settings.show_menu_bar && ImGui::BeginMainMenuBar()) {
            if(ImGui::BeginMenu("File")) {
            if(ImGui::MenuItem("Load ROM")) {
                file_dialog.open();
            }
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Emulation")) {
            if(ImGui::MenuItem("Pause", nullptr, rom_loaded && !emu_thread.running(), rom_loaded)) {
                if(emu_thread.running()) {
                    stopEmulation();
                } else {
                    startEmulation();
                }
            }

            if(ImGui::MenuItem("Next frame", nullptr, nullptr, rom_loaded && !emu_thread.running())) {
                core->run(280896);
            }

            if(ImGui::MenuItem("Fast forward", nullptr, emu_thread.fastforwarding(), rom_loaded)) {
                emu_thread.setFastforward(!emu_thread.fastforwarding());
            }

            if(ImGui::MenuItem("Reset", nullptr, nullptr, rom_loaded)) {
                resetEmulation();
            }

            if(ImGui::MenuItem("Stop", nullptr, nullptr, rom_loaded)) {
                stopEmulation();
                core->bus.pak.unload();
                emu_thread.setFastforward(false);
                video_device.clear(0);
                audio_device.clear();
                rom_loaded = false;
            }
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Options")) {
            ImGui::MenuItem("Settings", nullptr, &settings_window.active);
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Metrics", nullptr, &metrics_window.active);
            ImGui::MenuItem("ROM Info", nullptr, &rom_info_window.active);
            ImGui::Separator();
            drawSizeMenuItems();

            ImGui::EndMenu();
        }
        
        if(ImGui::BeginMenu("Help")) {
            if(ImGui::MenuItem("About")) {
                about_dialog.open();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if(rom_loaded) {
        ImGui::SetNextWindowSize(ImVec2(width, height - frame_height));
        ImGui::SetNextWindowPos(ImVec2(0, settings.show_menu_bar ? ImGui::GetFrameHeight() : 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize;
        flags |= ImGuiWindowFlags_NoDecoration;
        flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;

        if(ImGui::Begin("##GBA Screen", nullptr, flags)) {
            drawContextMenu();
            ImVec2 available = ImGui::GetContentRegionAvail();
            
            ImGui::SetCursorPos(ImVec2((available.x - screen_width) * 0.5f, (available.y - screen_height) * 0.5f));
            ImGui::Image((void*)(intptr_t)video_device.getTextureID(), ImVec2(screen_width, screen_height));
        }
        ImGui::End();

        if(settings.show_status_bar && ImGui::BeginViewportSideBar("##StatusBar", ImGui::GetMainViewport(), ImGuiDir_Down, ImGui::GetFrameHeight(), ImGuiWindowFlags_MenuBar)) {
            if(ImGui::BeginMenuBar()) {
                ImGui::Text("GBA FPS: %.1f", gba_fps);
                ImGui::SetCursorPosX(ImGui::CalcTextSize("GBA FPS: ____._ ").x);
                ImGui::Text("Speed: %.0f%%", gba_fps / 59.7275f * 100.0f);
                ImGui::EndMenuBar();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    } else {
        ImVec2 padding = ImGui::GetStyle().WindowPadding;
        
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGui::SetNextWindowSize(ImVec2(width, height - (settings.show_menu_bar ? ImGui::GetFrameHeight() : 0)));
        ImGui::SetNextWindowPos(ImVec2(0, settings.show_menu_bar ? ImGui::GetFrameHeight() : 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize;
        flags |= ImGuiWindowFlags_NoDecoration;
        flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;

        if(ImGui::Begin("Game List", nullptr, flags)) {
            drawContextMenu();
            ImGui::SetCursorPos(padding);

            if(ImGui::Button("Refresh")) {
                refreshGameList();
            }

            ImGui::SameLine();
            ImGui::Text("%zu items", game_list.size());
            ImGui::Separator();

            ImGui::BeginChild("##GameList_List");
            drawContextMenu();
            ImGuiTableFlags table_flags = ImGuiTableFlags_RowBg;
            if(ImGui::BeginTable("##GameList_Table", 2, table_flags)) {
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

                        unsigned int size = std::filesystem::file_size(game_list[i]) / 1_KiB;
                        ImGui::Text("%u KiB", size);
                    }
                }
                ImGui::EndTable();
            }
            ImGui::EndChild();
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }


    for(auto ui_window : ui_windows) {
        if(ui_window->active) {
            ui_window->draw(*this);
        }
    }

    ImGui::PopStyleVar(2);

    endFrame();
}

void Frontend::beginFrame() {
    // start = std::chrono::steady_clock::now();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if(input_device.update(settings)) {
        //Gamepad got disconnected
        settings.input_source = 0;
    }
}

void Frontend::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    //Frame times
    auto now = std::chrono::steady_clock::now();
    auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(now - start);
    start = now;

    frame_times.push((float)frame_time.count() / 1000.0f);

    //Calculate framerate as a moving average of the last 100 frames
    average_fps = 0;
    size_t host_size = 0;
    for(size_t i = 0; i < 100; i++) {
        if(frame_times.peek(i) != 0) {
            average_fps += frame_times.peek(i);
            host_size++;
        }
    }
    // (1 / (average / 100)) * 1000
    average_fps = (1000.0f * host_size) / average_fps;

    //Calculate framerate as a moving average of the last 100 frames
    const auto &gba_frame_times = core->debug.getFrameTimes();
    gba_fps = 0;
    for(size_t i = 0; i < gba_frame_times.size(); i++) {
        if(gba_frame_times.peek(i) != 0) {
            gba_fps += gba_frame_times.peek(i);
        }
    }
    gba_fps = (1000.0f * gba_frame_times.size()) / gba_fps;
    // LOG_INFO("Speed: {}%", (m_gba_fps / 59.7275f) * 100.0f);

    float temp[100];
    frame_times.copy(temp);
    metrics_window.setGUIFrameTimes(temp);
    gba_frame_times.copy(temp);
    metrics_window.setGBAFrameTimes(temp);
    audio_buffer_sizes.copy(temp);
    metrics_window.setAudioBufferSizes(temp);
    core->debug.getCPUUsage().copy(temp);
    metrics_window.setCPUUsage(temp);
}

void Frontend::audioCallback(bool new_samples, float *samples, size_t buffer_size) {
    emu_thread.sendCommand(RUN);
    audio_buffer_sizes.push(buffer_size);

    if(!new_samples) {
        return;
    }

    audio_buffer_mutex.lock();
    std::memcpy(audio_samples_l, samples, 750 * sizeof(float));
    std::memcpy(audio_samples_r, samples + 750, 750 * sizeof(float));
    audio_buffer_mutex.unlock();
}

void Frontend::windowSizeCallback(GLFWwindow *window, int width, int height) {
    CallbackUserData *user_data = reinterpret_cast<CallbackUserData*>(glfwGetWindowUserPointer(window));

    if(user_data != nullptr) {
        Frontend *instance = reinterpret_cast<Frontend*>(user_data->frontend);
        instance->width = width;
        instance->height = height;

        //Redraw gui
        instance->refreshScreenDimensions();
        
        if(!instance->just_changed_windowed_size) {
            //Redrawing the interface immediately after setting the size or going fullscreen/windowed causes a crash
            instance->drawInterface();
        } else {
            instance->just_changed_windowed_size = false;
        }
        
        glfwSwapBuffers(window);
    }
}