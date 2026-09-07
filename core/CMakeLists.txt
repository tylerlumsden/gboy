add_library(core STATIC
    gameboy.cpp
    cartridge.cpp
    cpu.cpp
    timer.cpp
    ppu.cpp
)

target_compile_definitions(core PUBLIC LOG_LEVEL=${LOG_LEVEL})

target_link_libraries(core PRIVATE gbcommon)

target_include_directories(core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)