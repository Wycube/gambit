#pragma once

#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wmicrosoft-enum-value"
    #pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#elif defined(__GNUG__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#include "common/Types.hpp"
#include <fmt/format.h>


namespace common {

namespace log {

enum LogLevel : u8 {
    LEVEL_DEBUG   = 0x01,
    LEVEL_TRACE   = 0x02,
    LEVEL_INFO    = 0x04,
    LEVEL_WARNING = 0x08,
    LEVEL_ERROR   = 0x10,
    LEVEL_FATAL   = 0x20
};

void set_log_filter(u8 filter);
void log(const std::string &message, LogLevel level);
void log_debug(const std::string &message, LogLevel level, const char *file, const char *func, int line);

} //namespace log

#define LOG_MESSAGE(level, message, ...) common::log::log(fmt::format(message, ##__VA_ARGS__), level)
#define DEBUG_LOG(level, message, ...) common::log::log_debug(fmt::format(message, ##__VA_ARGS__), level, __FILE__, __func__, __LINE__)

#define LOG_DEBUG(message, ...)     LOG_MESSAGE(common::log::LEVEL_DEBUG, message, ##__VA_ARGS__)
#define LOGD_DEBUG(message, ...)    DEBUG_LOG(common::log::LEVEL_DEBUG, message, ##__VA_ARGS__)
#define LOG_TRACE(message, ...)     LOG_MESSAGE(common::log::LEVEL_TRACE, message, ##__VA_ARGS__)
#define LOGD_TRACE(message, ...)    DEBUG_LOG(common::log::LEVEL_TRACE, message, ##__VA_ARGS__)
#define LOG_INFO(message, ...)      LOG_MESSAGE(common::log::LEVEL_INFO, message, ##__VA_ARGS__)
#define LOGD_INFO(message, ...)     DEBUG_LOG(common::log::LEVEL_INFO, message, ##__VA_ARGS__)
#define LOG_WARNING(message, ...)   LOG_MESSAGE(common::log::LEVEL_WARNING, message, ##__VA_ARGS__)
#define LOGD_WARNING(message, ...)  DEBUG_LOG(common::log::LEVEL_WARNING, message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...)     LOG_MESSAGE(common::log::LEVEL_ERROR, message, ##__VA_ARGS__)
#define LOGD_ERROR(message, ...)    DEBUG_LOG(common::log::LEVEL_ERROR, message, ##__VA_ARGS__)
#define LOG_FATAL(message, ...)     LOG_MESSAGE(common::log::LEVEL_FATAL, message, ##__VA_ARGS__); std::exit(-1)
#define LOGD_FATAL(message, ...)    DEBUG_LOG(common::log::LEVEL_FATAL, message, ##__VA_ARGS__); std::exit(-1)

} //namespace common

#if defined(__clang__)
    #pragma clang diagnostic pop
#elif defined(__GNUG__)
    #pragma GCC diagnostic pop
#endif