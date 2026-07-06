project(qml-zstd-example
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)

qt_add_executable(qml-zstd-example
    main.cpp
)

qt_add_qml_module(qml-zstd-example
    URI QZstdExample
    VERSION 1.0
    SOURCES
    QML_FILES
        main.qml
        ZStdPage.qml
        ZStdCompressPage.qml
        ZStdDecompressPage.qml
        ZstdCrypto.qml
        About.qml
)

target_link_libraries(qml-zstd-example PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Concurrent
    Qt6::Quick
    Qt6::Qml
    ${LibSodium_LIBRARIES} JosephsOddBuilder_Crypto qt-sodium qml-sodium qml-sodiumplugin
    ${LibZstd_LIBRARIES}   JosephsOddBuilder_Zstd   qt-zstd   qml-zstd   qml-zstdplugin
)

install(TARGETS qml-zstd-example
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
