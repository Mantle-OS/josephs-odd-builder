project(qt-huggingface
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)

set(QHF_HEADERS
    huggingfaceapi.h                    # hugging face api callback functions all functions here return a QFuture<QJsonObject> |  90% done
    huggingfacefilemanifest.h           # file mainifest from huggings faces api calls | schema started
    huggingfacepackagemanifest.h        # package mainifest for huggingface api | schema started
    huggingfacecache.h                  # local cache storage for huggingface api | 0% done
    huggingfaceuser.h                   # the user storage area.
    huggingfaceusermanager.h            # the manager of all the users
    huggingfacehub.h                    # entry point | 0% done
    huggingfacerepoinfo.h
)

set(QHF_CXX
    huggingfaceapi.cpp
    huggingfacepackagemanifest.cpp
    huggingfacefilemanifest.cpp
    huggingfacecache.cpp
    huggingfaceuser.cpp
    huggingfaceusermanager.cpp
    huggingfacehub.cpp
    huggingfacerepoinfo.cpp
)

# set(QHF_QML ## later )

qt_add_library(qt-huggingface STATIC)

qt_add_qml_module(qt-huggingface
    URI QHF
    VERSION 1.0
    SOURCES
      ${QHF_HEADERS}
      ${QHF_CXX}
    # QML_FILES
    #   ${QHF_QML}
)


target_include_directories(qt-huggingface PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)

target_link_libraries(qt-huggingface PRIVATE
    ${LibSodium_LIBRARIES} JosephsOddBuilder_Crypto qt-sodium
    ${LibZstd_LIBRARIES}
    ${YAMLCpp_LIBRARIES}
    qt-ai-utils qml-ai-utils qml-ai-utilsplugin
    qt-session-manager
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Concurrent
    Qt6::Quick
    Qt6::Qml
)
