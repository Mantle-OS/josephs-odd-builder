project(qml-sodium-example
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)

set(SODIUM_EXAMPLE_QML
    main.qml
    SecureBox.qml
    CryptoSigner.qml
    HashView.qml
    KeyManager.qml
    PasswordUtils.qml
    HMacSha256.qml
)

qt_add_executable(qml-sodium-example main.cpp)

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
    # Qt6::qmlfolderlistmodelplugin

    ${LibSodium_LIBRARIES}
    JosephsOddBuilder_Crypto
    qt-sodium
    qml-sodium
    qml-sodiumplugin

    qt-ai-utils
    qml-ai-utils
    qml-ai-utilsplugin
)

install(TARGETS qml-sodium-example
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)