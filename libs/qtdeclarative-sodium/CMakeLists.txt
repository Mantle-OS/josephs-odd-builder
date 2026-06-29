project(qtdeclarative-sodium
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

set(QDEC_SODIUM_HEADERS
    qmlsodiumbox.h
    qmlsodiumpasswordutils.h
    qmlsodiumcryptosign.h
    qmlsodiumpasswordutils.h
    qmlsodiumhash.h
    qmlsodiumkeys.h
)

set(QDEC_SODIUM_CXX
    qmlsodiumbox.cpp
    qmlsodiumpasswordutils.cpp
    qmlsodiumcryptosign.cpp    
    qmlsodiumpasswordutils.cpp
    qmlsodiumhash.cpp
    qmlsodiumkeys.cpp
)

set(QDEC_SODIUM_QML
    SecureTextField.qml
)

qt_add_library(qtdeclarative-sodium STATIC)
qt_add_qml_module(qtdeclarative-sodium
    URI Sodium
    VERSION 1.0
    SOURCES
      ${QDEC_SODIUM_HEADERS}
      ${QDEC_SODIUM_CXX}
    QML_FILES
      ${QDEC_SODIUM_QML}
)


target_include_directories(qtdeclarative-sodium PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)

target_link_libraries(qtdeclarative-sodium PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    ${LibSodium_LIBRARIES}
    qt-sodium
    qt-ai-utils # why we include this
)
