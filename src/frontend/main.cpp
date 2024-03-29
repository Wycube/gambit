#include "common/Version.hpp"
#include "common/Types.hpp"
#include "common/Log.hpp"
#include "Frontend.hpp"
#include "common/File.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/gl.h>
#include <filesystem>
#include <fstream>
#include <vector>


auto init() -> GLFWwindow* {
    //Initialize GLFW and create window
    if(glfwInit() == GLFW_FALSE) {
        LOG_FATAL("GLFW failed to initialize!");
    }

    GLFWwindow *window = glfwCreateWindow(960, 661, "", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    //Load OpenGL functions with glad and GLFW
    int version = gladLoadGL(glfwGetProcAddress);
    if(version == 0) {
        LOG_FATAL("Glad failed to initialize!");
    }

    LOG_DEBUG("Loaded OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
    LOG_DEBUG(" Vender           : {}", glGetString(GL_VENDOR));
    LOG_DEBUG(" Renderer         : {}", glGetString(GL_RENDERER));
    LOG_DEBUG(" OpenGL Version   : {}", glGetString(GL_VERSION));
    LOG_DEBUG(" Shading Language : {}", glGetString(GL_SHADING_LANGUAGE_VERSION));

    return window;
}

void cleanup(GLFWwindow *window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

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
                fmt::print("Usage: gambit [options] rom_path\n");
                fmt::print("\n");
                fmt::print("-h --help     Prints help text\n");
                fmt::print("-v --version  Prints version information\n");
                fmt::print("-b FILE       Specify path to a BIOS binary (defaults to bios.bin)\n");
                fmt::print("-d            Enable debug level logging    (disabled by default)\n");
                fmt::print("-t            Enable trace level logging    (disabled by default)\n");
                fmt::print("-i            Disable info level logging    (enabled by default)\n");
                fmt::print("-w            Disable warning level logging (enabled by default)\n");
                fmt::print("-e            Disable error level logging   (enabled by default)\n");
                
                return 0;
            } else if(strcmp(substr, "v") == 0 || strcmp(substr, "-version") == 0) {
                fmt::print("Gambit, Game Boy Advance Emulator,\n");
                fmt::print("Copyright (c) 2021-2023 Wycube\n");
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

    LOG_DEBUG("Version : {}", common::GIT_DESC);
    LOG_DEBUG("Branch  : {}", common::GIT_BRANCH);
    LOG_DEBUG("Commit  : {}", common::GIT_COMMIT);

    GLFWwindow *window = init();
    Frontend app(window);
    app.init();
    
    Settings settings = app.getSettings();

    if(has_bios_path) {
        settings.bios_path = bios_path;
    } else {
        if(settings.bios_path.empty()) {
            settings.bios_path = "bios.bin";
        }
    }

    if(settings.rom_path.empty()) {
        if(has_rom_path) {
            settings.rom_path = std::filesystem::path(rom_path).parent_path().string();
        } else {
            settings.rom_path = std::filesystem::current_path().string();
        }
    }

    app.setSettings(settings);

    if(has_rom_path) {
        if(app.loadROM(rom_path)) {
            app.startEmulation();
        }
    }

    app.mainloop();
    app.shutdown();

    cleanup(window);
}