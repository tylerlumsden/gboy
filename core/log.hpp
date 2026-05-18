#pragma once

#include <format>
#include <fstream>

namespace Log {

enum Level {
    Forced = 2,
    Info = 1,
    Debug = 0
};

constexpr auto log_level_string(Level log_level) {
    switch(log_level) {
    case Level::Forced: return "FORCED";
    case Level::Info: return "INFO";
    case Level::Debug: return "DEBUG";
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
    auto& log = Logger::get_logger<LogLevel>();
    if constexpr(LogLevel >= LOG_LEVEL) {
        std::string message = std::format(info, std::forward<Args>(args)...);
        log << std::format("{}: {}\n", log_level_string(LogLevel), message);
    }
}

}