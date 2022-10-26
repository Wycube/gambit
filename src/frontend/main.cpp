#include "emulator/core/GBA.hpp"
#include "common/Version.hpp"
#include "common/Types.hpp"
#include "common/Log.hpp"
#include "Frontend.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/gl.h>
#include <miniaudio.h>
#include <filesystem>
#include <fstream>
#include <vector>


int main(int argc, char *argv[]) {
    bool has_rom_path = false, has_bios_path = false;
    std::string rom_path, bios_path;
    u8 log_filter = 0xFF & ~common::log::LEVEL_DEBUG & ~common::log::LEVEL_TRACE;

    for(int i = 1; i < argc; i++) {
        const char *str = argv[i];

        if(str[0] == '-') {
            if(strlen(str) < 2) {
                continue;
            }

            const char *substr = &argv[i][1];

            if(strcmp(substr, "h") == 0 || strcmp(substr, "-help") == 0) {
                fmt::print("Usage: emu [options] rom_path\n");
                fmt::print("\n");
                fmt::print("-h --help     Prints help text\n");
                fmt::print("-v --version  Prints version information\n");
                fmt::print("-b FILE       Specify path to a BIOS binary (defaults to bios.bin)\n");
                fmt::print("-d            Enable debug level logging    (disabled by default)\n");
                fmt::print("-t            Enable trace level logging    (disabled by default)\n");
                fmt::print("-i            Disable info level logging    (enabled by default)\n");
                fmt::print("-w            Disalbe warning level logging (enabled by default)\n");
                fmt::print("-e            Disable error level logging   (enabled by default)\n");
                
                return 0;
            } else if(strcmp(substr, "v") == 0 || strcmp(substr, "-version") == 0) {
                fmt::print("Game Boy Advance Emulator,\n");
                fmt::print("Copyright (c) 2021-2022 Wycube\n");
                fmt::print("\n");
                fmt::print("Version : {}\n", common::GIT_DESC);
                fmt::print("Branch  : {}\n", common::GIT_BRANCH);
                fmt::print("Commit  : {}\n", common::GIT_COMMIT);
                
                return 0;
            } else if(strcmp(substr, "b") == 0) {
                if(!has_bios_path && i + 1 < argc) {
                    has_bios_path = true;
                    bios_path = argv[++i];
                }
            } else if(strcmp(substr, "d") == 0) {
                log_filter |= common::log::LEVEL_DEBUG;
            } else if(strcmp(substr, "t") == 0) {
                log_filter |= common::log::LEVEL_TRACE;
            } else if(strcmp(substr, "i") == 0) {
                log_filter &= ~common::log::LEVEL_INFO;
            } else if(strcmp(substr, "w") == 0) {
                log_filter &= ~common::log::LEVEL_WARNING;
            } else if(strcmp(substr, "e") == 0) {
                log_filter &= ~common::log::LEVEL_ERROR;
            } else {
                LOG_WARNING("Unknown option '{}'", substr);
            }
        } else if(!has_rom_path) {
            rom_path = str;
            has_rom_path = true;
        }
    }

    common::log::set_log_filter(log_filter);

    if(!has_rom_path) {
        LOG_FATAL("No ROM file provided!");
    }

    if(!has_bios_path) {
        LOG_WARNING("No BIOS file specified! Using default: bios.bin");
        bios_path = "bios.bin";
    }

    LOG_DEBUG("Version : {}", common::GIT_DESC);
    LOG_DEBUG("Branch  : {}", common::GIT_BRANCH);
    LOG_DEBUG("Commit  : {}", common::GIT_COMMIT);

    std::ifstream rom_file(rom_path, std::ios_base::binary);
    std::ifstream bios_file(bios_path, std::ios_base::binary);

    if(!rom_file.is_open()) {
        LOG_FATAL("Unable to open ROM file {}!", rom_path);
    }

    if(!bios_file.is_open()) {
        LOG_FATAL("Unable to open BIOS file {}!", bios_path);
    }

    //Initialize GLFW and create Window
    if(glfwInit() == GLFW_FALSE) {
        LOG_FATAL("GLFW failed to initialize!");
    }
    
    GLFWwindow *window = glfwCreateWindow(1080, 720, "", 0, 0);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    //Initialize glad/OpenGL
    int version = gladLoadGL(glfwGetProcAddress);
    if(version == 0) {
        LOG_FATAL("Glad failed to initialize!");
    }

    LOG_DEBUG("Initialized OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
    LOG_DEBUG("Vender           : {}", glGetString(GL_VENDOR));
    LOG_DEBUG("Renderer         : {}", glGetString(GL_RENDERER));
    LOG_DEBUG("OpenGL Version   : {}", glGetString(GL_VERSION));
    LOG_DEBUG("Shading Language : {}", glGetString(GL_SHADING_LANGUAGE_VERSION));

    //Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    Frontend app(window);
    
    {
        size_t rom_size = std::filesystem::file_size(rom_path);
        size_t bios_size = std::filesystem::file_size(bios_path);
        
        std::vector<u8> rom(rom_size);
        std::vector<u8> bios(bios_size);
        
        rom_file.read(reinterpret_cast<char*>(rom.data()), rom_size);
        bios_file.read(reinterpret_cast<char*>(bios.data()), bios_size);

        LOG_INFO("ROM file {} ({} bytes) read", rom_path, rom_size);
        LOG_INFO("BIOS file {} ({} bytes) read", bios_path, bios_size);

        app.loadBIOS(bios);
        app.loadROM(std::move(rom));
    }

    rom_file.close();
    bios_file.close();

    std::string save_path = rom_path.substr(0, rom_path.find_last_of(".")) + ".sav";
    app.loadSave(save_path);
    app.init();

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        app.drawInterface();
        glfwSwapBuffers(window);
    }

    app.shutdown();
    app.writeSave(save_path);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}