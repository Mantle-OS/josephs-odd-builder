cmake_minimum_required(VERSION 3.16)
project(job-kms-basic-test
    VERSION 0.1
    DESCRIPTION "Show a white screen for 5 seconds using job_render and linux framebuffer"
    LANGUAGES CXX
)

add_executable(job-kms-basic-test
    main.cpp
)

target_compile_options(job-kms-basic-test PRIVATE
    -freflection
    -fcontracts)

target_link_options(job-kms-basic-test PRIVATE
    -fcontracts
)

set_target_properties(job-kms-basic-test PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(job-kms-basic-test
    PRIVATE
        Threads::Threads
        atomic
        JosephsOddBuilder_Core
        JosephsOddBuilder_Threads
        JosephsOddBuilder_IO
        JosephsOddBuilder_Render
        ${LibDrm_LIBRARIES}
        ${LibInput_LIBRARIES}
        ${LibUdev_LIBRARIES}
        ${OpenGl_LIBRARIES}
        # ${Harfbuzz_LIBRARIES}
        # ${XkbCommon_LIBRARIES}
)


install(TARGETS job-kms-basic-test
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

