cmake_minimum_required(VERSION 3.16)
project(gboy)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -g")

set(SOURCES
    rom.cpp
)
add_executable(main rom.cpp)