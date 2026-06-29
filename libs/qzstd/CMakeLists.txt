project(qt6-zstd
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)
option(QZSTD_SODIUM_SUPPORT "Adds supports for encryption and decryption" ON)

set(QTZSTD_HEADERS
    qt-zstd/qzstdio.h           ## QIoDevice Implementation
    qt-zstd/qzstdoptions.h      ## Base Core members
    qt-zstd/qzstdcompressor.h   ## non encyption zstd compressor
    qt-zstd/qzstddecompressor.h ## non encyption zstd decompressor
    qt-zstd/qzstd.h             ## Threads and API to the whole lib
)
set(QTZSTD_CXX
    qt-zstd/qzstdio.cpp
    qt-zstd/qzstdoptions.cpp
    qt-zstd/qzstdcompressor.cpp
    qt-zstd/qzstddecompressor.cpp
    qt-zstd/qzstd.cpp
)

if(QZSTD_SODIUM_SUPPORT)
    add_compile_definitions(QZSTD_SODIUM_SUPPORT=1)
    list(APPEND QTZSTD_CXX
        qt-zstd/qzstdcompressorcrypto.cpp       ## Compress with encryption
        qt-zstd/qzstddecompressorcrypto.cpp     ## DeCompress with encryption
        qt-zstd/qzstdsign.cpp                   ## sign and verify the intergrerty of the archive
    )

    list(APPEND QTZSTD_HEADERS
        qt-zstd/qzstdcompressorcrypto.h
        qt-zstd/qzstddecompressorcrypto.h
        qt-zstd/qzstdsign.h
    )
endif()

add_library(qt-zstd STATIC
    ${QTZSTD_HEADERS}
    ${QTZSTD_CXX}
)

target_link_libraries(qt-zstd PRIVATE
    Qt6::Core
    Qt6::Concurrent
    ${LibSodium_LIBRARIES} qt-sodium
    ${LibZstd_LIBRARIES}
)

target_include_directories(qt-zstd PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qt-zstd>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qt-zstd>
)


#===================================================================================================================================

set(QMLZSTD_HEADERS
    qml-zstd/qmlzstd.h
)
set(QMLZSTD_CXX
    qml-zstd/qmlzstd.cpp
)
# set(QMLZSTD_QML)
qt_add_library(qml-zstd STATIC)
qt_add_qml_module(qml-zstd
    URI QZstd
    VERSION 1.0
    SOURCES
      ${QMLZSTD_HEADERS}
      ${QMLZSTD_CXX}
    # QML_FILES
    #   ${QMLZSTD_QML}
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
    ${LibSodium_LIBRARIES}
    qt-sodium
    ${LibZstd_LIBRARIES}
    qt-zstd
)
