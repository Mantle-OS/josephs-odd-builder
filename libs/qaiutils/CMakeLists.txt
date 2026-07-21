project(qt6-ai-utils
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)

# ==================
# Qt side
# ==================
set(QAIUTILS_HEADERS
    qt-ai-utils/pointer-macros.h
    qt-ai-utils/property-macros.h
    qt-ai-utils/qaiutils.h
    qt-ai-utils/qjsonapiclient.h
    qt-ai-utils/qconcurrentprocess.h
    qt-ai-utils/qdownloader.h
)

set(QAIUTILS_SOURCES
    qt-ai-utils/qaiutils.cpp
    qt-ai-utils/qjsonapiclient.cpp
    qt-ai-utils/qconcurrentprocess.cpp
    qt-ai-utils/qdownloader.cpp
)

if(BUILD_SHARED_LIBS)
    qt_add_library(qt-ai-utils SHARED
        ${QAIUTILS_HEADERS}
        ${QAIUTILS_SOURCES}
    )
else()
    qt_add_library(qt-ai-utils STATIC
        ${QAIUTILS_HEADERS}
        ${QAIUTILS_SOURCES}
    )
endif()

set(QAIUTILS_EXPORT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/qaiutils_export.h")
generate_export_header(qt-ai-utils
    BASE_NAME QAIUTILS
    EXPORT_FILE_NAME "${QAIUTILS_EXPORT_HEADER}"
)

set_target_properties(qt-ai-utils PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "qaiutils"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(qt-ai-utils PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qt-ai-utils>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qt-ai-utils>   
    ## export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(qt-ai-utils PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Concurrent
)

install(TARGETS qt-ai-utils
    EXPORT QAiUtilsTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qt-ai-utils
)

install(FILES ${QAIUTILS_EXPORT_HEADER} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qt-ai-utils")
install(FILES ${QAIUTILS_HEADERS}       DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qt-ai-utils")

if(JOB_LINUX)
    set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/qtaiutils.pc")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/qt-ai-utils/qtaiutils.pc.in"
        "${PKGCONFIG_FILE}"
        @ONLY
    )
    install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
endif()

install(EXPORT QAiUtilsTargets
    FILE QAiUtilsTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QAiUtilsTargets
)

#################################################################################
# Start QML side
#################################################################################

set(QMLAIUTILS_HEADERS
    qml-ai-utils/objectmodel.h
    qml-ai-utils/qmlstringlist.h
    qml-ai-utils/qaipath.h
    qml-ai-utils/qaipathing.h
)

set(QMLAIUTILS_SOURCES
    qml-ai-utils/qmlstringlist.cpp
    qml-ai-utils/qaipath.cpp
    qml-ai-utils/qaipathing.cpp
)

## Elements and what not for later when stablized.
set(QMLAIUTILS_QML)

if(BUILD_SHARED_LIBS)
    qt_add_library(qml-ai-utils SHARED)
else()
    qt_add_library(qml-ai-utils STATIC)
endif()

qt_add_qml_module(qml-ai-utils
    URI QAi
    VERSION 1.0
    OUTPUT_TARGETS QMLAIUTILS_OUTPUT_TARGETS
    SOURCES
      ${QMLAIUTILS_HEADERS}
      ${QMLAIUTILS_SOURCES}
    # QML_FILES
    #   ${QMLAIUTILS_QML}
)

set(QMLAIUTILS_EXPORT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/qmlaiutils_export.h")
generate_export_header(qml-ai-utils
    BASE_NAME QMLAIUTILS
    EXPORT_FILE_NAME "${QMLAIUTILS_EXPORT_HEADER}"
)

target_include_directories(qml-ai-utils PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qml-ai-utils>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qml-ai-utils>
    ## export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(qml-ai-utils
    PUBLIC
        Qt6::Core
        Qt6::Qml
        qt-ai-utils
    PRIVATE
        Qt6::Network
        Qt6::Concurrent
        Qt6::Gui
        Qt6::Quick
)

set(QMLAIUTILS_INSTALL_DIR "${QML_INSTALL_DIR}/QAi")

install(TARGETS qml-ai-utils
    EXPORT QmlAiUtilsTargets
    LIBRARY DESTINATION     "${QMLAIUTILS_INSTALL_DIR}"
    ARCHIVE DESTINATION     "${QMLAIUTILS_INSTALL_DIR}"
    RUNTIME DESTINATION     "${QMLAIUTILS_INSTALL_DIR}"
    INCLUDES DESTINATION    "${CMAKE_INSTALL_INCLUDEDIR}/qml-ai-utils"
)
install(FILES ${QMLAIUTILS_EXPORT_HEADER}   DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qml-ai-utils")
install(FILES ${QMLAIUTILS_HEADERS}         DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qml-ai-utils")

