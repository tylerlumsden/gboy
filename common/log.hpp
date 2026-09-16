#pragma once

#include <format>
#include <fstream>
#include <utility>

namespace Log {

constexpr int LOGMAX = 4;

enum Level {
    Forced = LOGMAX,
    Error = 3,
    Info = 2,
    Debug = 1,
    Verbose = 0,
    Doctor = -1
};

constexpr auto log_level_string(Level log_level) {
    switch(log_level) {
    case Level::Forced: return "FORCED";
    case Level::Error: return "ERROR";
    case Level::Info: return "INFO";
    case Level::Debug: return "DEBUG";
    case Level::Verbose: return "VERBOSE";
    case Level::Doctor: return "DOCTOR";
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

template<Level MsgLevel>
void write_to_loggers(const std::string& message) {
    if constexpr(MsgLevel >= LOG_LEVEL) {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            if(MsgLevel >= LOG_LEVEL) {
                ((Logger::get_logger<static_cast<Level>(LOG_LEVEL + Is)>()
                << std::format("{}: {}\n", log_level_string(MsgLevel), message)), ...);
            }
        }(std::make_index_sequence<MsgLevel - LOG_LEVEL + 1>{});
    }
}

template<Level LogLevel, typename... Args>
void log(std::format_string<Args...> info, Args&&... args) {
    if constexpr(LogLevel == Level::Doctor && LogLevel >= LOG_LEVEL) {
        auto& log = Logger::get_logger<LogLevel>();
        std::string message = std::format(info, std::forward<Args>(args)...);
        log << std::format("{}\n", message);
    }
    else if constexpr(LogLevel >= LOG_LEVEL) {
        std::string message = std::format(info, std::forward<Args>(args)...);
        write_to_loggers<LogLevel>(message);
    }
}

}