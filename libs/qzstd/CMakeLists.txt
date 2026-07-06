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
set(QTZSTD_CXX
    qt-zstd/qzstdcompressor.cpp
    qt-zstd/qzstddecompressor.cpp
    qt-zstd/qzstdcompressorcrypto.cpp
    qt-zstd/qzstddecompressorcrypto.cpp
    qt-zstd/qzstdsign.cpp
    qt-zstd/qzstd.cpp
)

add_library(qt-zstd STATIC
    ${QTZSTD_HEADERS}
    ${QTZSTD_CXX}
)

target_link_libraries(qt-zstd PRIVATE
    Qt6::Core
    qt-ai-utils
    ${LibSodium_LIBRARIES} JosephsOddBuilder_Crypto qt-sodium
    ${LibZstd_LIBRARIES} JosephsOddBuilder_Zstd
)

target_include_directories(qt-zstd PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qt-zstd>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qt-zstd>
)


#===================================================================================================================================

set(QMLZSTD_HEADERS

    qml-zstd/qmlcompressor.h
    qml-zstd/qmlcryptcompressor.h

    qml-zstd/qmldecompressor.h
    qml-zstd/qmlcryptodecompressor.h

    qml-zstd/qmlzstd.h
)

set(QMLZSTD_CXX
    qml-zstd/qmlzstd.cpp
)
set(QMLZSTD_QML
    qml-zstd/ZStdOptionsPopup.qml
)
qt_add_library(qml-zstd STATIC)
qt_add_qml_module(qml-zstd
    URI QZstd
    VERSION 1.0
    SOURCES
      ${QMLZSTD_HEADERS}
      ${QMLZSTD_CXX}
    QML_FILES
      ${QMLZSTD_QML}
)

target_include_directories(qml-zstd PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qml-zstd>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qml-zstd>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qt-zstd>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qt-zstd>
)

target_link_libraries(qml-zstd PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    qt-ai-utils
    ${LibSodium_LIBRARIES} JosephsOddBuilder_Crypto qt-sodium qml-sodium qml-sodiumplugin
    ${LibZstd_LIBRARIES} JosephsOddBuilder_Zstd qt-zstd
)
