project(qt6-sodium
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)

# =======START Qt BINDINGS=================
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

if(BUILD_SHARED_LIBS)
    qt_add_library(qt-sodium SHARED
          ${QSODIUM_HEADERS}
          ${QSODIUM_CXX}
    )
else()
    qt_add_library(qt-sodium STATIC
          ${QSODIUM_HEADERS}
          ${QSODIUM_CXX}
    )
endif()

set(QTSODIUM_EXPORT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/qsodium_export.h")
generate_export_header(qt-sodium
    BASE_NAME QSODIUM
    EXPORT_FILE_NAME "${QTSODIUM_EXPORT_HEADER}"
)

set_target_properties(qt-sodium PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "qsodium"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(qt-sodium PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qt-sodium>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qt-sodium>
    ## export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(qt-sodium PRIVATE
    ${LibSodium_LIBRARIES}
    Qt6::Core
    JosephsOddBuilder_Crypto
)

install(TARGETS qt-sodium
    EXPORT QSodiumTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${QTSODIUM_EXPORT_HEADER}     DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qt-sodium")
install(FILES ${QSODIUM_HEADERS}            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qt-sodium")

if(JOB_LINUX)
    set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/qtsodium.pc")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/qt-sodium/qtsodium.pc.in"
        "${PKGCONFIG_FILE}"
        @ONLY
    )
    install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
endif()

install(EXPORT QSodiumTargets
    FILE QSodiumTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QSodiumTargets
)

# ============================================================================================
# START QML BINDINGS
# ============================================================================================
set(QDEC_SODIUM_HEADERS
    qml-sodium/qmlsodiumbox.h
    qml-sodium/qmlsodiumcryptosign.h
    qml-sodium/qmlsodiumpasswordutils.h
    qml-sodium/qmlsodiumhash.h
    qml-sodium/qmlsodiumkeys.h
    qml-sodium/qmlsecuremem.h
    qml-sodium/qmlsecurememinput.h
)

set(QDEC_SODIUM_CXX
    qml-sodium/qmlsodiumbox.cpp
    qml-sodium/qmlsodiumcryptosign.cpp
    qml-sodium/qmlsodiumpasswordutils.cpp
    qml-sodium/qmlsodiumhash.cpp
    qml-sodium/qmlsodiumkeys.cpp
    qml-sodium/qmlsecuremem.cpp
    qml-sodium/qmlsecurememinput.cpp
)

if(BUILD_SHARED_LIBS)
    qt_add_library(qml-sodium SHARED)
else()
    qt_add_library(qml-sodium STATIC)
endif()


qt_add_qml_module(qml-sodium
    URI Sodium
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Sodium"
    OUTPUT_TARGETS QML_SODIUM_OUTPUT_TARGETS
    SOURCES
      ${QDEC_SODIUM_HEADERS}
      ${QDEC_SODIUM_CXX}
    # QML_FILES
    #   ${QDEC_SODIUM_QML}
)
set(QMLSODIUM_EXPORT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/qmlsodium_export.h")
generate_export_header(qml-sodium
    BASE_NAME QMLSODIUM
    EXPORT_FILE_NAME "${QMLSODIUM_EXPORT_HEADER}"
)

set_target_properties(qml-sodium PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "qmlsodium"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(qml-sodium PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qml-sodium>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qml-sodium>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qt-sodium>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qt-sodium>
    ## export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(qml-sodium
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        Qt6::Qml
        JosephsOddBuilder_Crypto
        qt-sodium
        qt-ai-utils
    PRIVATE
        Qt6::QuickControls2
        ${LibSodium_LIBRARIES}
)

set(QML_SODIUM_INSTALL_DIR "${QML_INSTALL_DIR}/Sodium")
install(TARGETS
    qml-sodium
    ${QML_SODIUM_OUTPUT_TARGETS}
    EXPORT QmlSodiumTargets
    LIBRARY DESTINATION "${QML_SODIUM_INSTALL_DIR}"
    ARCHIVE DESTINATION "${QML_SODIUM_INSTALL_DIR}"
    RUNTIME DESTINATION "${QML_SODIUM_INSTALL_DIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qml-sodium"
)

install(FILES ${QMLSODIUM_EXPORT_HEADER}    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qml-sodium")
install(FILES ${QDEC_SODIUM_HEADERS}        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qml-sodium")

install(EXPORT QmlSodiumTargets
    FILE QmlSodiumTargets.cmake
    NAMESPACE JOB::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QmlSodium"
)