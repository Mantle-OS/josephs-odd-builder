project(qt-llama
    VERSION 0.1
    LANGUAGES CXX C
)

set(CMAKE_AUTOMOC ON)

set(QLLAMA_HEADERS
    qllamaenums.h
    qllamabase.h
    qllamamodelkvoverride.h
    qllamamodelparams.h
    qllamaadapterlora.h
    qllamacontextparams.h
    qllamaadapterlora.h
    qllamasampler.h
    qllmacontext.h
    qllamamodel.h
    qllamavocab.h
    # qllamatoken.h
    qllamaengine.h
    qllamalocalengine.h
    qllamaclientengine.h
    qllamaserverengine.h
    # qllamachat.h

)

set(QLLAMA_CXX
    qllamaenums.cpp
    qllamabase.cpp
    qllamamodelkvoverride.cpp
    qllamaadapterlora.cpp
    qllamamodelparams.cpp
    qllamacontextparams.cpp
    qllamasampler.cpp
    qllmacontext.cpp
    qllamamodel.cpp
    qllamavocab.cpp
    # qllamatoken.cpp
    # qllamachat.cpp
    qllamaengine.cpp
    qllamalocalengine.cpp
    qllamaclientengine.cpp
    qllamaserverengine.cpp
)

# set(QLLAMA_QML )

qt_add_library(qt-llama STATIC)
qt_add_qml_module(qt-llama
    URI QLlama
    VERSION 1.0
    SOURCES
      ${QLLAMA_HEADERS}
      ${QLLAMA_CXX}
    # QML_FILES
    #   ${QLLAMA_QML}
)

target_include_directories(qt-llama PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)

target_link_libraries(qt-llama PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Concurrent
    Qt6::Quick
    Qt6::Qml
    ${YAMLCpp_LIBRARIES}
    ${LibSodium_LIBRARIES}
    qt-sodium
    qt-ai-utils qml-ai-utils qml-ai-utilsplugin
    ggml
    llama llama-common llama-common-base
)
