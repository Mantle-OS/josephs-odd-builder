project(qllama-qml-client-server-example
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)

set(QHF_EXAMPLE_QML
)

qt_add_executable(qllama-qml-client-server-example main.cpp )

qt_add_qml_module(qllama-qml-client-server-example
    URI QLlamaExample
    VERSION 1.0
    QML_FILES
        main.qml
)

target_link_libraries(qllama-qml-client-server-example PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    ${YAMLCpp_LIBRARIES}
    ${LibSodium_LIBRARIES}
    qt-sodium
    qt-ai-utils qml-ai-utils qml-ai-utilsplugin
    ggml
    llama llama-common llama-common-base
    # ${LLGUIDANCE_STATIC_ARCHIVE}
    # job_llguidance
    qt-llama qt-llamaplugin
)

include(GNUInstallDirs)
install(TARGETS qllama-qml-client-server-example
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
