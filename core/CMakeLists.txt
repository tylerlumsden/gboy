add_library(core STATIC
    gameboy.cpp
    cartridge.cpp
    cpu.cpp
)

if(NOT DEFINED LOG_LEVEL)
    set(LOG_LEVEL 1)
endif()
target_compile_definitions(core PUBLIC LOG_LEVEL=${LOG_LEVEL})

target_include_directories(core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)