add_library(core STATIC
    gameboy.cpp
    rom.cpp
    cpu.cpp
)

if(NOT DEFINED LOG_LEVEL)
    set(LOG_LEVEL 0)
endif()
target_compile_definitions(core PRIVATE LOG_LEVEL=${LOG_LEVEL})

target_include_directories(core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)