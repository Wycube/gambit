#include "Log.hpp"
#include <chrono>


namespace common {

namespace log {

//Default to all log levels
static u8 s_log_filter = 0xFF;


void set_log_filter(u8 filter) {
    s_log_filter = filter;
}

void log(const std::string &message, LogLevel level) {
    if((s_log_filter & level) == 0) {
        return;
    }

    std::string prefix = "\33[";

    switch(level) {
        case LEVEL_DEBUG :   prefix += "34m[D]"; break;
        case LEVEL_TRACE :   prefix += "35m[T]"; break;
        case LEVEL_INFO :    prefix += "32m[I]"; break;
        case LEVEL_WARNING : prefix += "33m[W]"; break;
        case LEVEL_ERROR :   prefix += "91m[E]"; break;
        case LEVEL_FATAL :   prefix += "31m[F]"; break;
    }

    fmt::print("{} {}\33[39m\n", prefix, message);
}

void log_debug(const std::string &message, LogLevel level, const char *file, const char *func, int line) {
    log(fmt::format("{}:{}:{}(): {}", file, line, func, message), level);
}

} //namespace log

} //namespace common