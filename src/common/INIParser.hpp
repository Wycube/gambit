#pragma once

#include <string>
#include <vector>
#include <unordered_map>


namespace common {

struct IniMap {
    std::unordered_map<std::string, size_t> sections;
    std::vector<std::unordered_map<std::string, std::string>> values;
};

auto loadIniFile(const std::string &path) -> IniMap;
void writeIniFile(const IniMap &map, const std::string &path);

} //namespace common