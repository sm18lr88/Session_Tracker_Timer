cmake_minimum_required(VERSION 3.15)
project(WolfTimer VERSION 1.0.0 LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(MSVC)
    # Use Unicode
    add_definitions(-DUNICODE -D_UNICODE)

    # Suppress deprecation warnings for standard C functions
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

    # Enable warning level 4
    add_compile_options(/W4)

    # Use static runtime for easier distribution
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_subdirectory(src)
