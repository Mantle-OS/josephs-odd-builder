cmake_minimum_required(VERSION 3.19)
project(qsodium-example LANGUAGES CXX)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core)

qt_standard_project_setup()

qt_add_executable(qsodium-example
    main.cpp
)

target_link_libraries(qsodium-example PRIVATE
    Qt::Core
    ${LibSodium_LIBRARIES}
    qt-sodium
)

install(TARGETS qsodium-example
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
