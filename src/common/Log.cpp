#include "Log.hpp"
#include <chrono>


namespace common {

// static log::Logger s_logger;

namespace log {

// Logger::Logger() {
//     m_thread = std::thread(&Logger::log_to_file, this);
//     m_log_file.open("log.txt", std::ios_base::out);
// }

// Logger::~Logger() {
//     m_mutex.lock();
//     m_stop = true;
//     m_mutex.unlock();
//     m_cv.notify_one();

//     m_thread.join();
//     m_log_file.close();
// }

// void Logger::log_to_file() {
//     while(true) {
//         //Log the latest queue entry to the console
//         std::unique_lock lock(m_mutex);
//         m_cv.wait(lock, [this] { return m_new_message || m_stop; });
//         printf("Yeet I'm running");

//         if(m_new_message) {
//             //m_log_file << m_queue.front();
//             fmt::print(m_queue.front());
//             m_queue.pop();
        
//             if(m_queue.empty()) {
//                 m_new_message = false;
//             }
//         }

//         if(m_stop) {
//             break;
//         }
//     }
// }

// void Logger::queueMessage(std::string &&message) {
//     m_mutex.lock();
//     m_queue.push(std::move(message));
//     m_new_message = true;
//     m_mutex.unlock();
//     m_cv.notify_one();
// }

// void log_message(std::string &&message) {
//     static Logger s_logger;

//     s_logger.queueMessage(std::move(message));
// }

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
    //log_message(std::move(fmt::format("{} {}\33[39m\n", prefix, message)));
}

void log_debug(const std::string &message, LogLevel level, const char *file, const char *func, int line) {
    log(fmt::format("{}:{}:{}(): {}", file, line, func, message), level);
}

} //namespace log

} //namespace common