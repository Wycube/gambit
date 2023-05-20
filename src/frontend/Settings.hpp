#pragma once

#include <string>


struct Settings {
    bool show_status_bar = false;

    std::string rom_path;
    std::string bios_path;
    bool skip_bios = false;

    int screen_filter = 0;

    float volume = 0.0f;

    bool enable_debugger = false;

    auto operator==(const Settings &other) -> bool {
        return show_status_bar == other.show_status_bar &&
            rom_path == other.rom_path &&
            bios_path == other.bios_path &&
            skip_bios == other.skip_bios &&
            screen_filter == other.screen_filter &&
            volume == other.volume &&
            enable_debugger == other.enable_debugger;
    }

    auto operator!=(const Settings &other) -> bool {
        return !(*this == other);
    }
};