project(qt-llama
    VERSION 0.1
    LANGUAGES CXX C
)

set(CMAKE_AUTOMOC ON)

set(QLLAMA_HEADERS
    qllamaenums.h
    qllamabase.h
    qllamamodelkvoverride.h
    qllamamodeltensorbuftoverride.h

    # qlla

    qllamamodelparams.h
    qllamaadapterlora.h
    qllamacontextparams.h
    qllamasampler.h
    qllmacontext.h
    qllamamodel.h
    qllamavocab.h
    qllamaengine.h
    qllamalocalengine.h
    qllamaclientengine.h
    qllamaserverengine.h
)

set(QLLAMA_CXX
    qllamaenums.cpp
    qllamabase.cpp
    qllamamodelkvoverride.cpp
    qllamamodeltensorbuftoverride.cpp


    qllamaadapterlora.cpp
    qllamamodelparams.cpp
    qllamacontextparams.cpp
    qllamasampler.cpp
    qllmacontext.cpp
    qllamamodel.cpp
    qllamavocab.cpp
    qllamaengine.cpp
    qllamalocalengine.cpp
    qllamaclientengine.cpp
    qllamaserverengine.cpp
)

# set(QLLAMA_QML)

if(BUILD_SHARED_LIBS)
    qt_add_library(qt-llama SHARED)
else()
    qt_add_library(qt-llama STATIC)
    add_compile_definitions(QLLAMA_STATIC_DEFINE)
endif()

qt_add_qml_module(qt-llama
    URI QLlama
    VERSION 1.0
    OUTPUT_TARGETS QLLAMA_OUTPUT_TARGETS
    SOURCES
        ${QLLAMA_HEADERS}
        ${QLLAMA_CXX}
    # QML_FILES
    #     ${QLLAMA_QML}
)

set(QLLAMA_EXPORT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/qllama_export.h")
generate_export_header(qt-llama
    BASE_NAME QLLAMA
    EXPORT_FILE_NAME "${QLLAMA_EXPORT_HEADER}"
)

set_target_properties(qt-llama PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "qllama"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(qt-llama PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qt-llama>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(qt-llama
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        Qt6::Qml
        qt-ai-utils
        qml-ai-utils
        Qt6::Concurrent
        Qt6::Network
    PRIVATE
        ${YAMLCpp_LIBRARIES}
        ${LibSodium_LIBRARIES}
        qt-sodium
        # qml-ai-utilsplugin
        $<BUILD_INTERFACE:ggml>
        $<BUILD_INTERFACE:llama>
        $<BUILD_INTERFACE:llama-common>
        $<BUILD_INTERFACE:llama-common-base>
)

set(QLLAMA_INSTALL_DIR "${QML_INSTALL_DIR}/QLlama")
install(TARGETS qt-llama
    ${QLLAMA_OUTPUT_TARGETS}
    EXPORT QLlamaTargets
    LIBRARY DESTINATION  "${QLLAMA_INSTALL_DIR}"
    ARCHIVE DESTINATION  "${QLLAMA_INSTALL_DIR}"
    RUNTIME DESTINATION  "${QLLAMA_INSTALL_DIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qt-llama"
)

install(FILES ${QLLAMA_EXPORT_HEADER} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qt-llama")
install(FILES ${QLLAMA_HEADERS}       DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qt-llama")

if(BUILD_SHARED_LIBS)
    install(FILES ${QLLAMA_QML} DESTINATION "${QLLAMA_INSTALL_DIR}")
endif()

if(JOB_LINUX)
    set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/qt-llama.pc")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/qtllama.pc.in"
        "${PKGCONFIG_FILE}"
        @ONLY
    )
    install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
endif()

install(EXPORT QLlamaTargets
    FILE QLlamaTargets.cmake
    NAMESPACE JOB::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QLlama"
)