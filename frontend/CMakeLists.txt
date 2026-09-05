add_library(frontend STATIC
    frontend.cpp
)

target_compile_definitions(frontend PUBLIC LOG_LEVEL=${LOG_LEVEL})

find_package(SDL3 REQUIRED CONFIG)
target_link_libraries(frontend PUBLIC SDL3::SDL3)

target_include_directories(frontend
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)