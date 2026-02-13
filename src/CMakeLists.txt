set(SOURCES
    CoverSquareWindow.cpp
    main.cpp
    SetupDialog.cpp
    TimerWindow.cpp
)

set(HEADERS
    CoverSquareWindow.h
    resource.h
    SetupDialog.h
    TimerState.h
    TimerWindow.h
)

set(RESOURCES
    resources.rc
)

add_executable(WolfTimer WIN32
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_include_directories(WolfTimer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(WolfTimer PRIVATE
    comctl32
    uxtheme
)

set_target_properties(WolfTimer PROPERTIES
    OUTPUT_NAME "Wolf-Timer"
    VS_DPI_AWARE "PerMonitor"
)

set_property(TARGET WolfTimer PROPERTY VS_USER_PROPS
    "${CMAKE_CURRENT_SOURCE_DIR}/no_auto_manifest.props"
)

if(MSVC)
    target_link_options(WolfTimer PRIVATE "/MANIFEST:NO")
endif()

add_custom_command(TARGET WolfTimer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:WolfTimer>"
    "${CMAKE_SOURCE_DIR}/Wolf-Timer.exe"
    COMMENT "Copying executable to project root"
)
