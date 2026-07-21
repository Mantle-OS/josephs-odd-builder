project(qt6-zstd
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)

set(QTZSTD_HEADERS
    qt-zstd/qzstdoptions.h                  ## Base Core members
    qt-zstd/qzstdcompressor.h               ## non encyption zstd compressor
    qt-zstd/qzstddecompressor.h             ## non encyption zstd decompressor
    qt-zstd/qzstdcompressorcrypto.h         ## Compress with encryption
    qt-zstd/qzstddecompressorcrypto.h       ## DeCompress with encryption
    qt-zstd/qzstdsign.h                     ## sign and verify the intergrerty of the archive
    qt-zstd/qzstd.h                         ## Threads and API to the whole lib
)
set(QTZSTD_SOURCES
    qt-zstd/qzstdcompressor.cpp
    qt-zstd/qzstddecompressor.cpp
    qt-zstd/qzstdcompressorcrypto.cpp
    qt-zstd/qzstddecompressorcrypto.cpp
    qt-zstd/qzstdsign.cpp
    qt-zstd/qzstd.cpp
)
if(BUILD_SHARED_LIBS)
    add_library(qt-zstd SHARED
        ${QTZSTD_HEADERS}
        ${QTZSTD_SOURCES}
    )
else()
    add_library(qt-zstd STATIC
        ${QTZSTD_HEADERS}
        ${QTZSTD_SOURCES}
    )
    target_compile_definitions(qt-zstd PUBLIC QZSTD_STATIC_DEFINE)
endif()

if(JOB_WINDOWS)
    target_compile_options(qt-zstd PRIVATE /wd4251)
endif()

set(QTZSTD_EXPORT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/qzstd_export.h")
generate_export_header(qt-zstd
    BASE_NAME QZSTD
    EXPORT_FILE_NAME "${QTZSTD_EXPORT_HEADER}"
)

set_target_properties(qt-zstd PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "qzstd"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(qt-zstd PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qt-zstd>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qt-zstd>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(qt-zstd
    PUBLIC
        Qt6::Core
        JosephsOddBuilder_Zstd
        JosephsOddBuilder_Crypto
        qt-sodium
        qt-ai-utils
    PRIVATE
        ${LibZstd_LIBRARIES}
        ${LibSodium_LIBRARIES}
)

install(TARGETS qt-zstd
    EXPORT QZstdTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qt-zstd
)

install(FILES ${QTZSTD_EXPORT_HEADER}     DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qt-zstd")
install(FILES ${QTZSTD_HEADERS}           DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qt-zstd")

if(JOB_LINUX)
    set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/qtzstd.pc")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/qt-zstd/qtzstd.pc.in"
        "${PKGCONFIG_FILE}"
        @ONLY
    )
    install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
endif()

install(EXPORT QZstdTargets
    FILE QZstdTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QZstdTargets
)

#===================================================================================================================================

set(QMLZSTD_HEADERS
    qml-zstd/qmlcompressor.h
    qml-zstd/qmlcryptcompressor.h
    qml-zstd/qmldecompressor.h
    qml-zstd/qmlcryptodecompressor.h
    qml-zstd/qmlzstd.h
)

set(QMLZSTD_SOURCES
    qml-zstd/qmlzstd.cpp
)
set(QMLZSTD_QML
    qml-zstd/ZStdOptionsPopup.qml
)

if(BUILD_SHARED_LIBS)
    qt_add_library(qml-zstd SHARED)
else()
    qt_add_library(qml-zstd STATIC)
    target_compile_definitions(qml-zstd PUBLIC QMLZSTD_STATIC_DEFINE)
endif()

qt_add_qml_module(qml-zstd
    URI QZstd
    VERSION 1.0
    OUTPUT_TARGETS QML_ZSTD_OUTPUT_TARGETS
    SOURCES
      ${QMLZSTD_HEADERS}
      ${QMLZSTD_SOURCES}
    QML_FILES
      ${QMLZSTD_QML}
)

if(JOB_WINDOWS)
    target_compile_options(qml-zstd PRIVATE /wd4251)
endif()

set(QMLZSTD_EXPORT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/qmlzstd_export.h")
generate_export_header(qml-zstd
    BASE_NAME QMLZSTD
    EXPORT_FILE_NAME "${QMLZSTD_EXPORT_HEADER}"
)

set_target_properties(qml-zstd PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "qmlzstd"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(qml-zstd PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qml-zstd>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qml-zstd>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qt-zstd>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qt-zstd>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(qml-zstd
    PUBLIC
        Qt6::Core
        Qt6::Quick
        Qt6::Qml
        qt-ai-utils
        qt-sodium
        qml-sodium
        JosephsOddBuilder_Zstd
        qt-zstd
    PRIVATE
        Qt6::Gui
        Qt6::QuickControls2
        ${LibSodium_LIBRARIES}
        JosephsOddBuilder_Crypto
        ${LibZstd_LIBRARIES}
)

set(QML_ZSTD_INSTALL_DIR "${QML_INSTALL_DIR}/QZstd")

install(TARGETS
    qml-zstd
    ${QML_ZSTD_OUTPUT_TARGETS}
    EXPORT QmlZstdTargets
    LIBRARY DESTINATION "${QML_ZSTD_INSTALL_DIR}"
    ARCHIVE DESTINATION "${QML_ZSTD_INSTALL_DIR}"
    RUNTIME DESTINATION "${QML_ZSTD_INSTALL_DIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qml-zstd"
)

install(FILES ${QMLZSTD_EXPORT_HEADER}  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qml-zstd")
install(FILES ${QMLZSTD_HEADERS}        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qml-zstd")

install(EXPORT QmlZstdTargets
    FILE QmlZstdTargets.cmake
    NAMESPACE JOB::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QmlZstd"
)
