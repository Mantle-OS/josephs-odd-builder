project(qml-stable-diffusion
    VERSION 0.1
    LANGUAGES CXX C
)
set(CMAKE_AUTOMOC ON)

set(QMLSD_HEADERS
    qsdenums.h
    qsdbaseparam.h
    qsdtilingparams.h
    qsdembedding.h
    qsdctxparams.h
    qsdaudio.h
    qsdimage.h
    qsdslgparams.h
    qsdguidanceparams.h
    qsdsampleparams.h
    qsdpmparams.h
    qsdcacheparams.h
    qsdlora.h
    qsdhiresparams.h
    qsdpulidparams.h
    qsdimggenparams.h
    qsdvidgenparams.h
    qsdbackenddevice.h
    qsdupscaleparams.h
    qsdctx.h
)


set(QMLSD_CXX
    qsdbaseparam.cpp
    qsdenums.cpp
    qsdtilingparams.cpp
    qsdembedding.cpp
    qsdctxparams.cpp
    qsdaudio.cpp
    qsdimage.cpp
    qsdslgparams.cpp
    qsdguidanceparams.cpp
    qsdsampleparams.cpp
    qsdpmparams.cpp
    qsdcacheparams.cpp
    qsdlora.cpp
    qsdhiresparams.cpp
    qsdpulidparams.cpp
    qsdimggenparams.cpp
    qsdvidgenparams.cpp
    qsdbackenddevice.cpp
    qsdupscaleparams.cpp
    qsdctx.cpp
)

set(QMLSD_QML
    QmlSdSampleParams.qml
    QmlSdGuidanceParams.qml
    QmlSdSlgParams.qml
    QmlSdLora.qml
    QmlSdPmParams.qml
    QmlSdTilingParams.qml
    QmlSdCacheParams.qml
    QmlSdHiResParams.qml
    QmlSdEmbedding.qml
    QmlSdCtxParams.qml
    QmlSdImgGenParams.qml
    QmlSdBackendManager.qml
    QmlSdAudio.qml
    QmlSdLoraManager.qml
)

# this will be later
# set(QSD_PRESETS_QML)

if(BUILD_SHARED_LIBS)
    qt_add_library(qml-stable-diffusion SHARED)
else()
    qt_add_library(qml-stable-diffusion STATIC)
    add_compile_definitions(QSD_STATIC_DEFINE)
endif()

qt_add_qml_module(qml-stable-diffusion
    URI QSd
    VERSION 1.0
    OUTPUT_TARGETS QML_SD_OUTPUT_TARGETS
    SOURCES
      ${QMLSD_HEADERS}
      ${QMLSD_CXX}
    QML_FILES
      ${QMLSD_QML}
)

set(QMLSD_EXPORT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/qmlsd_export.h")
generate_export_header(qml-stable-diffusion
    BASE_NAME QMLSD
    EXPORT_FILE_NAME "${QMLSD_EXPORT_HEADER}"
)

set_target_properties(qml-stable-diffusion PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "qmlsd"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    # POSITION_INDEPENDENT_CODE ON
)

target_include_directories(qml-stable-diffusion PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qml-stable-diffusion>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(qml-stable-diffusion
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        Qt6::Qml
        qt-ai-utils
        qml-ai-utils
        JosephsOddBuilder_Core
    PRIVATE
        ${YAMLCpp_LIBRARIES}
        $<BUILD_INTERFACE:stable-diffusion>
        $<BUILD_INTERFACE:ggml>
        Qt6::Concurrent
        JosephsOddBuilder_Simd
)

set(QML_SD_INSTALL_DIR "${QML_INSTALL_DIR}/QmlSd")
install(TARGETS qml-stable-diffusion
    ${QML_SD_OUTPUT_TARGETS}
    EXPORT QmlSdTargets
    LIBRARY DESTINATION "${QML_SD_INSTALL_DIR}"
    ARCHIVE DESTINATION "${QML_SD_INSTALL_DIR}"
    RUNTIME DESTINATION "${QML_SD_INSTALL_DIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qml-stable-diffusion"
)

install(FILES ${QMLSD_EXPORT_HEADER}    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qml-stable-diffusion")
install(FILES ${QMLSD_HEADERS}          DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qml-stable-diffusion")

if(BUILD_SHARED_LIBS)
    install(FILES ${QMLSD_QML}           DESTINATION "${QML_SD_INSTALL_DIR}")
endif()

if(JOB_LINUX)
    set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/qml-stable-diffusion.pc")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/qtstablediffusion.pc.in"
        "${PKGCONFIG_FILE}"
        @ONLY
    )
    install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
endif()

install(EXPORT QmlSdTargets
    FILE QmlSdTargets.cmake
    NAMESPACE JOB::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QmlSd"
)
