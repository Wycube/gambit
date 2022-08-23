#include "emulator/core/GBA.hpp"
#include "common/Version.hpp"
#include "common/Types.hpp"
#include "common/Log.hpp"
#include "DebuggerUI.hpp"
#include "device/OGLVideoDevice.hpp"
#include "device/GLFWInputDevice.hpp"
#include "Application.hpp"
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
#include <thread>
#include <signal.h>


static emu::GBA *s_gba;

void signal_handler(int signal) {
    LOG_ERROR("Signal {} received", signal);

    // emu::dbg::Debugger &debug = s_gba->getDebugger();
    // bool thumb = debug.getCPUCPSR() & emu::FLAG_THUMB;
    // u32 pc = debug.getCPURegister(15) - (thumb ? 2 : 4);

    // LOG_DEBUG("PC: {:08X} | Instruction: {:08X} | Disassembly: {}", pc, thumb ? debug.read16(pc) : debug.read32(pc), thumb ? debug.thumbDisassembleAt(pc) : debug.armDisassembleAt(pc));
    std::_Exit(-2);
}

static int s_counter = 0;

void audio_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count) {
    reinterpret_cast<std::atomic<bool>*>(device->pUserData)->store(true);
}

int main(int argc, char *argv[]) {
    LOG_INFO("Version: {}", common::GIT_DESC);
    LOG_INFO("Commit: {}", common::GIT_COMMIT);
    LOG_INFO("Branch: {}", common::GIT_BRANCH);

    if(argc < 2) {
        LOG_FATAL("No ROM file specified!");
    }

    std::string bios_path;

    if(argc < 3) {
        LOG_WARNING("No BIOS path specified! Using default: bios.bin");
        bios_path = "bios.bin";
    } else {
        bios_path = argv[2];
    }

    std::fstream rom_file(argv[1], std::ios_base::in | std::ios_base::binary);
    std::fstream bios_file(bios_path, std::ios_base::in | std::ios_base::binary);

    if(!rom_file.good()) {
        LOG_FATAL("ROM file not good!");
    }

    if(!bios_file.good()) {
        LOG_FATAL("BIOS file not good!");
    }

    size_t rom_size = std::filesystem::file_size(argv[1]);
    size_t bios_size = std::filesystem::file_size(bios_path);
    LOG_INFO("ROM Size: {}", rom_size);
    LOG_INFO("BIOS Size: {}", bios_size);

    signal(SIGSEGV, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGABRT, signal_handler);

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

    LOG_INFO("Loaded OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    //Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    Application app(window);

    //Miniaudio Testing
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = 48000;
    config.dataCallback = audio_callback;
    config.pUserData = app.getShouldRun();

    ma_device device;
    if(ma_device_init(nullptr, &config, &device) != MA_SUCCESS) {
        LOG_FATAL("Could not initalize Miniaudio device!");
    }
    if(ma_device_start(&device) != MA_SUCCESS) {
        LOG_FATAL("Could not start Miniaudio device!");
    }

    
    {
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

        //Close file streams
        rom_file.close();
        bios_file.close();

        app.loadROM(std::move(rom));
        app.loadBIOS(bios);
    }

    app.init();

    s_gba = app.getCore();

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        app.drawInterface();
        glfwSwapBuffers(window);
    }

    app.shutdown();

    ma_device_uninit(&device);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}