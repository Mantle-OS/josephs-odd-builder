cmake_minimum_required(VERSION 3.19)
project(qzstd-example LANGUAGES CXX)

option(QZSTD_SODIUM_SUPPORT "Adds supports for encryption and decryption" ON)
if(QZSTD_SODIUM_SUPPORT)
    add_compile_definitions(QZSTD_SODIUM_SUPPORT=1)
endif()

qt_standard_project_setup()
qt_add_executable(qzstd-example
    main.cpp
)

target_link_libraries(qzstd-example PRIVATE
    Qt::Core
    Qt::Concurrent
    ${LibSodium_LIBRARIES} JosephsOddBuilder_Crypto qt-sodium
    qt-zstd
)

install(TARGETS qzstd-example
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
