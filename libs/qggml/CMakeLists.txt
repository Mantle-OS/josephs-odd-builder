project(qt-ggml
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.2 COMPONENTS
    Core
    Gui
    Quick
    QuickControls2
    Qml
  REQUIRED
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LibSodium  REQUIRED libsodium)

set(QGGML_HEADERS
    qggmldevice.h
)

set(QGGML_CXX
    qggmldevice.cpp
)

set(QGGML_QML
)

qt_add_library(qt-ggml STATIC)
qt_add_qml_module(qt-ggml
    URI QGGML
    VERSION 1.0
    SOURCES
      ${QGGML_HEADERS}
      ${QGGML_CXX}
    QML_FILES
      ${QGGML_QML}
)


target_include_directories(qt-ggml PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)

target_link_libraries(qt-ggml PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    ${LibSodium_LIBRARIES}
    qt-sodium
    qt-stable-diffusion-utils # why we include this
    ggml
)
