cmake_minimum_required(VERSION 3.16)

project(job-kms-damage-quadrant-test
    VERSION 0.1
    DESCRIPTION "Test KMS framebuffer quadrant damage using DRM damage clips"
    LANGUAGES CXX
)

add_executable(job-kms-damage-quadrant-test
    main.cpp
)

target_compile_options(job-kms-damage-quadrant-test PRIVATE
    -freflection
    -fcontracts
)

target_link_options(job-kms-damage-quadrant-test PRIVATE
    -fcontracts
)

set_target_properties(job-kms-damage-quadrant-test PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(job-kms-damage-quadrant-test
    PRIVATE
        Threads::Threads
        atomic
        JosephsOddBuilder_Core
        JosephsOddBuilder_Threads
        JosephsOddBuilder_IO
        JosephsOddBuilder_Render
        PkgConfig::LibDrm
        ${LibInput_LIBRARIES}
        ${LibUdev_LIBRARIES}
        ${OpenGl_LIBRARIES}
        # ${Harfbuzz_LIBRARIES}
        # ${XkbCommon_LIBRARIES}
)

install(TARGETS job-kms-damage-quadrant-test
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
