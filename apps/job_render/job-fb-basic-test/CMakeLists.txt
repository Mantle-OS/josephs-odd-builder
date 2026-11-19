cmake_minimum_required(VERSION 3.16)
project(job-fb-basic-test
    VERSION 0.1
    DESCRIPTION "Show a white screen for 5 seconds using job_render and kms"
    LANGUAGES CXX
)

add_executable(job-fb-basic-test
    main.cpp
)

target_compile_options(job-fb-basic-test PRIVATE
    -freflection
    -fcontracts)

target_link_options(job-fb-basic-test PRIVATE
    -fcontracts
)

set_target_properties(job-fb-basic-test PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(job-fb-basic-test
    PRIVATE
        Threads::Threads
        atomic
        JosephsOddBuilder_Core
        JosephsOddBuilder_Threads
        JosephsOddBuilder_IO
        JosephsOddBuilder_Render
        PkgConfig::LibDrm
)


install(TARGETS job-fb-basic-test
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

