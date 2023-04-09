#pragma once

#include "common/Types.hpp"
#include <string>
#include <vector>


namespace movie {

struct BK2Movie {
    std::vector<u16> inputs;
};

auto loadBK2Movie(const std::string &path) -> BK2Movie;

} //namespace movie