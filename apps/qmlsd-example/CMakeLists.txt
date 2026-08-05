project(qt-stable-diffusion-example
    VERSION 0.1
    LANGUAGES CXX C
)

set(CMAKE_AUTOMOC ON)

set(QSD_EXAMPLE_QML
    main.qml
    SDThree.qml
    SDThreeMedium.qml
    ZImageTurbo.qml
)

qt_add_executable(qt-stable-diffusion-example main.cpp )

qt_add_qml_module(qt-stable-diffusion-example
    URI QMLSDEXAMPLE
    VERSION 1.0
    SOURCES
    QML_FILES
      ${QSD_EXAMPLE_QML}
)

# qt-huggingface
# qt-stable-diffusion
target_link_libraries(qt-stable-diffusion-example
    PUBLIC
        ${YAMLCpp_LIBRARIES}
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::Concurrent
        Qt6::Quick
        Qt6::Qml
        qt-ai-utils qml-ai-utils qml-ai-utilsplugin
        # qt-huggingfaceplugin
        qml-stable-diffusion qml-stable-diffusionplugin
    PRIVATE
        $<BUILD_INTERFACE:stable-diffusion>
        $<BUILD_INTERFACE:ggml>
)

install(TARGETS qt-stable-diffusion-example
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
