cmake_minimum_required(VERSION 3.15)
project(SessionTimer VERSION 1.0.0 LANGUAGES CXX)

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

# Source files
set(SOURCES
    main.cpp
    SetupDialog.cpp
    TimerWindow.cpp
)

# Header files
set(HEADERS
    resource.h
    TimerState.h
    SetupDialog.h
    TimerWindow.h
)

# Resource file
set(RESOURCES
    resources.rc
)

# Create Windows GUI executable (WIN32 suppresses console window)
add_executable(SessionTimer WIN32
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

# Link required Windows libraries
target_link_libraries(SessionTimer PRIVATE
    comctl32
    uxtheme
)

# Set output name - disable auto-manifest generation since we have our own in resources.rc
set_target_properties(SessionTimer PROPERTIES
    OUTPUT_NAME "SessionTimer"
    VS_DPI_AWARE "PerMonitor"
)

# Disable CMake's auto-generated manifest since we have one in resources.rc
set_property(TARGET SessionTimer PROPERTY VS_USER_PROPS "${CMAKE_CURRENT_SOURCE_DIR}/no_auto_manifest.props")

# For MSVC, disable default manifest
if(MSVC)
    target_link_options(SessionTimer PRIVATE "/MANIFEST:NO")
endif()

# Copy executable to project root for easy access (optional)
add_custom_command(TARGET SessionTimer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:SessionTimer>"
    "${CMAKE_SOURCE_DIR}/SessionTimer.exe"
    COMMENT "Copying executable to project root"
)
