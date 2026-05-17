#pragma once

#include <format>
#include <iostream>

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

template<Level LogLevel, typename... Args>
void log(std::format_string<Args...> info, Args&&... args) {
    if constexpr(LogLevel >= LOG_LEVEL) {
        std::string message = std::format(info, std::forward<Args>(args)...);
        std::cout << std::format("{}: {}\n", log_level_string(LogLevel), message);
    }
}

}