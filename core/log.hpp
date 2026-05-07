#pragma once

#include <format>
#include <iostream>

namespace Log {

template<typename... Args>
void log(std::format_string<Args...> info, Args&&... args) {
    std::cout << std::format(info, std::forward<Args>(args)...) << "\n";
}

}