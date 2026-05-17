#pragma once

#include <format>
#include <iostream>

namespace Log {

enum Level {
    Forced = 2,
    Info = 1,
    Debug = 0
};

template<Level LogLevel, typename... Args>
void log(std::format_string<Args...> info, Args&&... args) {
    if constexpr(LogLevel >= LOG_LEVEL) {
        std::cout << std::format(info, std::forward<Args>(args)...);
    }
}

}