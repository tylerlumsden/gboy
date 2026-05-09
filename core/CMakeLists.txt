add_library(core STATIC
    gameboy.cpp
    rom.cpp
    cpu.cpp
)

target_include_directories(core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)