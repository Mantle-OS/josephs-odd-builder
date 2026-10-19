cmake_minimum_required(VERSION 3.16)
project(job-inference-tool-test
    VERSION 0.1
    DESCRIPTION "Ask Qwen3 4b it the for a fast sqrt in c++"
    LANGUAGES CXX
)

add_executable(job-inference-tool-test
    main.cpp
)

target_compile_options(job-inference-tool-test PRIVATE
    -freflection
    -fcontracts)

target_link_options(job-inference-tool-test PRIVATE
    -fcontracts
)

set_target_properties(job-inference-tool-test PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(job-inference-tool-test
    PRIVATE
        Threads::Threads
        $<BUILD_INTERFACE:ggml>
        JosephsOddBuilder_Core
        JosephsOddBuilder_Threads
        JosephsOddBuilder_Ggml
        JosephsOddBuilder_Token
        JosephsOddBuilder_Model
        ${JOB_CUDA_LIBS}
)


install(TARGETS job-inference-tool-test
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

