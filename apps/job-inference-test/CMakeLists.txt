cmake_minimum_required(VERSION 3.16)
project(job-inference-test
    VERSION 0.1
    DESCRIPTION "Ask Qwen3 4b it the for a fast sqrt in c++"
    LANGUAGES CXX
)

add_executable(job-inference-test
    main.cpp
)

target_link_libraries(job-inference-test
    PRIVATE
        Threads::Threads
        $<BUILD_INTERFACE:ggml>
        JosephsOddBuilder_Core
        JosephsOddBuilder_Threads
        JosephsOddBuilder_Ggml
        JosephsOddBuilder_Token
        JosephsOddBuilder_Model

)

install(TARGETS job-inference-test
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

