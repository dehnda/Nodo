cmake_minimum_required(VERSION 3.20)

# Nodo CLI - Headless execution tool
project(nodo_cli VERSION 0.1.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CLI executable (no Qt dependency!)
add_executable(nodo_cli
    main.cpp
)

# Link only to nodo_core (no nodo_studio, no Qt)
target_link_libraries(nodo_cli
    PRIVATE
        nodo_core
)

target_compile_features(nodo_cli PRIVATE cxx_std_20)

# Compiler warnings
if(MSVC)
    target_compile_options(nodo_cli PRIVATE /W4)
else()
    target_compile_options(nodo_cli PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install target
install(TARGETS nodo_cli
    RUNTIME DESTINATION bin
)
