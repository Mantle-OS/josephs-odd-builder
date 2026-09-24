project(JosephOddBuild_WaylandProtocols
    VERSION 0.1
    DESCRIPTION "Wayland protocol support for Joseph's Odd Builder"
    LANGUAGES C CXX
)

set(JOB_LAYER_SHELL_SCHEMA
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/job-layer-shell-unstable-v1.xml"
)

job_wayland_scanner_lib(
    NAME job_wayland_protocols
    VERSION ${PROJECT_VERSION}
    DESCRIPTION "${PROJECT_DESCRIPTION}"
    CLIENT_SCHEMA_FILES
        ${JOB_LAYER_SHELL_SCHEMA}
    PRIVATE_SCHEMA_FILES
        ${JOB_LAYER_SHELL_SCHEMA}
    HEADERS
        test.h
    SOURCES
        test.cpp
    DEPENDENCIES
        ${WaylandClient_LIBRARIES}
    TARGET_EXTRA_INCLUDE
        ${WaylandClient_INCLUDE_DIRS}
)
