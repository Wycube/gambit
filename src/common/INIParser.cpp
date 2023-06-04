#include "INIParser.hpp"
#include <filesystem>
#include <fstream>


namespace common {

auto trimOuterWhitespace(const std::string &str) -> std::string {
    size_t first_char = std::string::npos;
    size_t last_char = std::string::npos;

    if(str.empty()) {
        return "";
    }

    for(size_t i = 0; i < str.size(); i++) {
        if(first_char == std::string::npos && str[i] != ' ' && str[i] != '\t') {
            first_char = i;
        }

        if(last_char == std::string::npos && str[str.size() - 1 - i] != ' ' && str[str.size() - 1 - i] != '\t') {
            last_char = str.size() - i;
        }

        if(first_char != std::string::npos && last_char != std::string::npos) {
            break;
        }
    }

    //String is entirely whitespace
    if(first_char == std::string::npos || last_char == std::string::npos) {
        return "";
    }
    
    return str.substr(first_char, last_char - first_char);
}

auto loadIniFile(const std::string &path) -> IniMap {
    IniMap map{};

    if(!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        return map;
    }

    std::ifstream file(path);

    if(!file.is_open()) {
        return map;
    }

    std::string line;
    size_t current_section = 0;

    while(std::getline(file, line)) {
        if(line.empty()) {
            continue;
        }

        //Section header
        if(line.at(0) == '[' && line.at(line.size() - 1) == ']' && line.size() > 2) {
            if(!map.sections.empty()) {
                current_section++;
            }

            map.sections[line.substr(1, line.size() - 2)] = current_section;
            map.values.push_back({});
        }

        //Key-Value Pair
        size_t equal_pos = line.find_first_of('=');
        
        if(equal_pos == std::string::npos || map.values.empty()) {
            continue;
        }

        std::string key = trimOuterWhitespace(line.substr(0, equal_pos));
        std::string value = trimOuterWhitespace(line.substr(equal_pos + 1, line.size() - equal_pos - 1));
        map.values[current_section][key] = value;
    }

    return map;
}

void writeIniFile(const IniMap &map, const std::string &path) {
    std::ofstream file(path);

    if(!file.is_open()) {
        return;
    }

    for(const auto section : map.sections) {
        file << '[' << section.first << "]\n";

        for(const auto value : map.values[section.second]) {
            file << value.first << " = " << value.second << '\n';
        }

        file << '\n';
    }
}

} //namespace common