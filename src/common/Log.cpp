#include "Log.hpp"

#include <chrono>


namespace common {

// static log::Logger s_logger;

namespace log {

// void Logger::log_to_file() {
//     while(true) {
//         //Log the latest queue entry to the console
//         {
//             std::unique_lock lock(m_message_mutex);

//             if(m_new_message) {
//                 while(!m_queue.empty()) {
//                     m_log_file << m_queue.front();
//                     //fmt::print("{}", m_queue.front());
//                     m_queue.pop();
//                 }

//                 m_new_message = false;
//             }

//         }

//         std::unique_lock lock(m_stop_mutex);
//         if(m_stop) {
//             break;
//         }
//     }
// }

// void Logger::init() {
//     if(m_thread.joinable()) {
//         fmt::print("Error: Logger already initialized!\n");
//         return;
//     }

//     m_thread = std::thread(&Logger::log_to_file, this);

//     if(m_log_file.is_open()) {
//         fmt::print("Error: Log file already opened!\n");
//         return;
//     }

//     m_log_file.open("log.txt", std::ios_base::out);
// }

// void Logger::queueMessage(const std::string &message) {
//     std::unique_lock lock(m_message_mutex);
//     m_queue.push(message);
//     m_new_message = true;
// }

// void Logger::close() {
//     if(!m_thread.joinable()) {
//         fmt::print("Error: Logger not initialized!\n");
//         return;
//     }

//     //End the thread's logging loop
//     {
//         std::unique_lock lock(m_stop_mutex);
//         m_stop = true;
//     }

//     m_thread.join();

//     if(!m_log_file.is_open()) {
//         printf("Error: Log file not open!");
//         return;
//     }

//     m_log_file.close();
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
    //s_logger.queueMessage(fmt::format("{} {}\33[39m\n", prefix, message));
}

void log_debug(const std::string &message, LogLevel level, const char *file, const char *func, int line) {
    log(fmt::format("{}:{}:{}(): {}", file, line, func, message), level);
}

} //namespace log

} //namespace common