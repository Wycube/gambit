#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmicrosoft-enum-value"
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#include <fmt/format.h>
// #include <thread>
// #include <queue>
// #include <fstream>
// #include <condition_variable>


namespace common {

namespace log {

enum LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

// class Logger final {
// private:

//     std::thread m_thread;
//     std::fstream m_log_file;
//     std::queue<std::string> m_queue;
//     std::mutex m_mutex;
//     std::condition_variable m_cv;
//     bool m_stop = false;
//     bool m_new_message = false;

//     void log_to_file();

// public:

//     Logger();
//     ~Logger();

//     void queueMessage(std::string &&message);
// };


void log(const std::string &message, LogLevel level);
void log_debug(const std::string &message, LogLevel level, const char *file, const char *func, int line);

} //namespace log

// extern log::Logger s_logger;

#define LOG_MESSAGE(level, message, ...) common::log::log(fmt::format(message, ##__VA_ARGS__), level)
#define DEBUG_LOG(level, message, ...) common::log::log_debug(fmt::format(message, ##__VA_ARGS__), level, __FILE__, __func__, __LINE__)

#define ____DEBUG
#if defined(____DEBUG)
    #define LOG_DEBUG(message, ...)     LOG_MESSAGE(common::log::DEBUG, message, ##__VA_ARGS__)
    #define LOGD_DEBUG(message, ...)    DEBUG_LOG(common::log::DEBUG, message, ##__VA_ARGS__)
#else
    #define LOG_DEBUG(message, ...)
    #define LOGD_DEBUG(message, ...)
#endif

#define LOG_INFO(message, ...)      LOG_MESSAGE(common::log::INFO, message, ##__VA_ARGS__)
#define LOGD_INFO(message, ...)     DEBUG_LOG(common::log::INFO, message, ##__VA_ARGS__)
#define LOG_WARNING(message, ...)   LOG_MESSAGE(common::log::WARNING, message, ##__VA_ARGS__)
#define LOGD_WARNING(message, ...)  DEBUG_LOG(common::log::WARNING, message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...)     LOG_MESSAGE(common::log::ERROR, message, ##__VA_ARGS__)
#define LOGD_ERROR(message, ...)    DEBUG_LOG(common::log::ERROR, message, ##__VA_ARGS__)
#define LOG_FATAL(message, ...)     LOG_MESSAGE(common::log::FATAL, message, ##__VA_ARGS__); std::exit(-1)
#define LOGD_FATAL(message, ...)    DEBUG_LOG(common::log::FATAL, message, ##__VA_ARGS__); std::exit(-1)

} //namespace common

#pragma clang diagnostic pop