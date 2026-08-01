cmake_minimum_required(VERSION 3.19)
project(qllama-simple-example LANGUAGES CXX C)

qt_standard_project_setup()

qt_add_executable(qllama-simple-example
    main.cpp
)

target_link_libraries(qllama-simple-example PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::Concurrent
    Qt6::Network
    ${YAMLCpp_LIBRARIES}
    ${LibSodium_LIBRARIES}
    qt-sodium
    qt-ai-utils qml-ai-utils qml-ai-utilsplugin
    ggml
    llama llama-common llama-common-base
    qt-llama
)

install(TARGETS qllama-simple-example
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
