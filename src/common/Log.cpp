#include "Log.hpp"


namespace common {

namespace log {

void log(const std::string &message, LogLevel level) {
    std::string prefix = "\33[";

    switch(level) {
        case DEBUG : prefix += "34m[D]"; break;
        case INFO : prefix += "32m[I]"; break;
        case WARNING : prefix += "33m[W]"; break;
        case ERROR : prefix += "91m[E]"; break;
        case FATAL : prefix += "31m[F]"; break;
    }

    fmt::print("{} {}\33[39m\n", prefix, message);
}

void log_debug(const std::string &message, LogLevel level, const char *file, const char *func, int line) {
    log(fmt::format("{}:{}:{}(): {}", file, line, func, message), level);
}

} //namespace log

} //namespace common