project(qt6-session-manager
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)

set(QTSESS_HEADERS
    qt-session-manager/qaitypes.h
    qt-session-manager/qaisessionworker.h
    qt-session-manager/qaiusersession.h
    qt-session-manager/qaisessionvault.h
    qt-session-manager/qaisessionuser.h
    qt-session-manager/qaisessionmanager.h
)

set(QTSESS_CXX
    qt-session-manager/qaisessionworker.cpp
    qt-session-manager/qaiusersession.cpp
    qt-session-manager/qaisessionvault.cpp
    qt-session-manager/qaisessionuser.cpp
    qt-session-manager/qaisessionmanager.cpp
)

add_library(qt-session-manager STATIC
    ${QTSESS_HEADERS}
    ${QTSESS_CXX}
)

target_link_libraries(qt-session-manager
    PUBLIC
        Qt6::Core
        qt-ai-utils
        qt-sodium
        qt-zstd
        aisession_lib
    PRIVATE
        Qt6::Concurrent
        ${LibSodium_LIBRARIES} JosephsOddBuilder_Crypto
        ${LibZstd_LIBRARIES}
)

target_include_directories(qt-session-manager PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qt-session-manager>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qt-session-manager>
)


#===================================================================================================================================

set(QMLSESS_HEADERS
    # qml-session-manager/.... which will be inherit of qaisessionmanager.cpp which will will start to expose things
)
set(QMLSESS_CXX

)
# set(QMLSESS_QML)
qt_add_library(qml-session-manager STATIC)
qt_add_qml_module(qml-session-manager
    URI QAiSessionManager
    VERSION 1.0
    SOURCES
      ${QMLSESS_HEADERS}
      ${QMLSESS_CXX}
    # QML_FILES
    #   ${QMLSESS_QML}
)

target_include_directories(qml-session-manager PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qml-session-manager>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qml-session-manager>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qt-session-manager>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qt-session-manager>
)

target_link_libraries(qml-session-manager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    qt-ai-utils qml-ai-utils qml-ai-utilsplugin
    ${LibSodium_LIBRARIES} JosephsOddBuilder_Crypto qt-sodium qml-sodium qml-sodiumplugin
    ${LibZstd_LIBRARIES} qt-zstd qml-zstdplugin
    aisession_lib
)
