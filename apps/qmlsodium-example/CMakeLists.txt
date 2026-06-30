project(qml-sodium-example
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)

set(SODIUM_EXAMPLE_QML
    main.qml
    SecureBox.qml    # showing how QmlSodiumBox Element works
    CryptoSigner.qml # showing how QmlSodiumCryptoSign works
    HashView.qml     # showing how QmlSodiumHash singleton works
    KeyManager.qml   # showing how QmlSodiumKeys Element works
    PasswodUtils.qml # showing how QmlSodiumPasswordUtils Element
)

qt_add_executable(qml-sodium-example main.cpp )

qt_add_qml_module(qml-sodium-example
    URI QmlSodiumExample
    VERSION 1.0
    SOURCES
    QML_FILES
      ${SODIUM_EXAMPLE_QML}
)

target_link_libraries(qml-sodium-example PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    ${LibSodium_LIBRARIES}
    qt-ai-utils qml-ai-utils qml-ai-utilsplugin
    qt-sodium qtdeclarative-sodium qtdeclarative-sodiumplugin
)

include(GNUInstallDirs)
install(TARGETS qml-sodium-example
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
