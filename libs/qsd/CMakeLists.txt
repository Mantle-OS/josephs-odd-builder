project(qt-stable-diffusion
    VERSION 0.1
    LANGUAGES CXX C
)
set(CMAKE_AUTOMOC ON)

set(QSD_HEADERS
    qsdenums.h
    qsdbaseparam.h
    qsdtilingparams.h
    qsdembedding.h
    qsdaudio.h
    qsdimage.h
    qsdslgparams.h
    qsdguidanceparams.h
    qsdsampleparams.h
    qsdpmparams.h
    qsdcacheparams.h
    qsdlora.h
    qsdhiresparams.h
    qsdimggenparams.h
    qsdvidgenparams.h
    qsdbackenddevice.h
    qsdupscaleparams.h
    qsdctx.h
)


set(QSD_CXX
    qsdbaseparam.cpp
    qsdenums.cpp
    qsdtilingparams.cpp
    qsdembedding.cpp
    qsdctxparams.h
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
    qsdimggenparams.cpp
    qsdvidgenparams.cpp
    qsdbackenddevice.cpp
    qsdupscaleparams.cpp
    qsdctx.cpp
)

set(QSD_QML
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
)

qt_add_library(qt-stable-diffusion STATIC)
qt_add_qml_module(qt-stable-diffusion
    URI QSd
    VERSION 1.0
    SOURCES
      ${QSD_HEADERS}
      ${QSD_CXX}
    QML_FILES
      ${QSD_QML}
)

target_include_directories(qt-stable-diffusion PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)

target_link_libraries(qt-stable-diffusion PRIVATE
    ${YAMLCpp_LIBRARIES}
    ggml
    stable-diffusion
    qt-ai-utils qml-ai-utils
    # qt-huggingface
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Concurrent
    Qt6::Quick
    Qt6::Qml
)

install(TARGETS qt-stable-diffusion
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/qml/QSd)
