cmake_minimum_required(VERSION 3.19)
project(qzstd-example LANGUAGES CXX)

qt_standard_project_setup()
qt_add_executable(qzstd-example
    main.cpp
)

target_link_libraries(qzstd-example PRIVATE
    Qt6::Core
    qt-ai-utils
    ${LibSodium_LIBRARIES} JosephsOddBuilder_Crypto qt-sodium
    ${LibZstd_LIBRARIES} JosephsOddBuilder_Zstd qt-zstd
)

install(TARGETS qzstd-example
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
