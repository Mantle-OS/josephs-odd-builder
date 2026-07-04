project(qt6-sodium
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)

option(QSECUREMEM_ALLOW_STRING "Allow for gated strings" OFF)
if(QSECUREMEM_ALLOW_STRING)
    add_compile_definitions(QSECUREMEM_ALLOW_STRING=1)
endif()

set(QSODIUM_HEADERS
    qt-sodium/qsecuremem.h
    qt-sodium/qextrarandom.h
    qt-sodium/qsodiumkeys.h
    qt-sodium/qsodiumcryptosign.h
    qt-sodium/qsodiumpasswordutils.h
    qt-sodium/qsodiumsecretbox.h
    qt-sodium/qsodiumhash.h
    qt-sodium/qsodium.h
)

set(QSODIUM_CXX
    qt-sodium/qextrarandom.cpp
    qt-sodium/qsecuremem.cpp
    qt-sodium/qsodiumkeys.cpp
    qt-sodium/qsodiumcryptosign.cpp
    qt-sodium/qsodiumpasswordutils.cpp
    qt-sodium/qsodiumsecretbox.cpp
    qt-sodium/qsodiumhash.cpp
    qt-sodium/qsodium.cpp
)

qt_add_library(qt-sodium STATIC
      ${QSODIUM_HEADERS}
      ${QSODIUM_CXX}
)

target_include_directories(qt-sodium PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qt-sodium>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qt-sodium>
)

target_link_libraries(qt-sodium PRIVATE
    ${LibSodium_LIBRARIES}
    Qt6::Core
    JosephsOddBuilder_Crypto
)


# =======START QML BINDINGS=================

set(QDEC_SODIUM_HEADERS
    qml-sodium/qmlsodiumbox.h
    qml-sodium/qmlsodiumpasswordutils.h
    qml-sodium/qmlsodiumcryptosign.h
    qml-sodium/qmlsodiumpasswordutils.h
    qml-sodium/qmlsodiumhash.h
    qml-sodium/qmlsodiumkeys.h
    qml-sodium/qmlsecuremem.h
    qml-sodium/qmlsecurememinput.h
)

set(QDEC_SODIUM_CXX
    qml-sodium/qmlsodiumbox.cpp
    qml-sodium/qmlsodiumpasswordutils.cpp
    qml-sodium/qmlsodiumcryptosign.cpp
    qml-sodium/qmlsodiumpasswordutils.cpp
    qml-sodium/qmlsodiumhash.cpp
    qml-sodium/qmlsodiumkeys.cpp
    qml-sodium/qmlsecuremem.cpp
    qml-sodium/qmlsecurememinput.cpp
)

# set(QDEC_SODIUM_QML
#     qml-sodium/SecureTextField.qml
# )

qt_add_library(qml-sodium STATIC)
qt_add_qml_module(qml-sodium
    URI Sodium
    VERSION 1.0
    SOURCES
      ${QDEC_SODIUM_HEADERS}
      ${QDEC_SODIUM_CXX}
    # QML_FILES
    #   ${QDEC_SODIUM_QML}
)


target_include_directories(qml-sodium PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qml-sodium>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qml-sodium>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qt-sodium>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qt-sodium>
)

target_link_libraries(qml-sodium PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    ${LibSodium_LIBRARIES} JosephsOddBuilder_Crypto qt-sodium
    qt-ai-utils
)
