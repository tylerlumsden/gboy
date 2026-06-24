#pragma once

#include <format>
#include <fstream>

namespace Log {

constexpr int LOGMAX = 5;

enum Level {
    Doctor = LOGMAX,
    Forced = 4,
    Error = 3,
    Info = 2,
    Debug = 1,
    Verbose = 0
};

constexpr auto log_level_string(Level log_level) {
    switch(log_level) {
    case Level::Doctor: return "DOCTOR";
    case Level::Forced: return "FORCED";
    case Level::Error: return "ERROR";
    case Level::Info: return "INFO";
    case Level::Debug: return "DEBUG";
    case Level::Verbose: return "VERBOSE";
    default: return "UNKNOWN LOG LEVEL";
    }
}

struct Logger {
    template<Level LogLevel>
    static std::ofstream& get_logger() {
        static std::ofstream log_file(std::format("{}.log", log_level_string(LogLevel)));
        return log_file;
    }
};

template<Level LogLevel, typename... Args>
void log(std::format_string<Args...> info, Args&&... args) {
    if constexpr(LogLevel == Level::Doctor) {
        auto& log = Logger::get_logger<LogLevel>();
        std::string message = std::format(info, std::forward<Args>(args)...);
        log << std::format("{}\n", message);
    }
    else if constexpr(LogLevel >= LOG_LEVEL) {
        auto& log = Logger::get_logger<LogLevel>();
        std::string message = std::format(info, std::forward<Args>(args)...);
        log << std::format("{}: {}\n", log_level_string(LogLevel), message);
    }
}

}