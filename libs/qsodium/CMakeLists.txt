project(qt-sodium
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)

option(QSECUREMEM_ALLOW_STRING "Allow for gated strings" ON)
if(QSECUREMEM_ALLOW_STRING)
    add_compile_definitions(QSECUREMEM_ALLOW_STRING=1)
endif()

set(QSODIUM_HEADERS
    qsodiuminit.h
    qextrarandom.h
    qsecuremem.h
    qsodiumkeys.h
    qsodiumcryptosign.h
    qsodiumpasswordutils.h
    qsodiumsecretbox.h
    qsodiumhash.h
    qsodium.h
)

set(QSODIUM_CXX
    qextrarandom.cpp
    qsecuremem.cpp
    qsodiumkeys.cpp
    qsodiuminit.cpp
    qsodiumcryptosign.cpp
    qsodiumpasswordutils.cpp
    qsodiumsecretbox.cpp
    qsodiumhash.cpp
    qsodium.cpp
)

qt_add_library(qt-sodium STATIC
      ${QSODIUM_HEADERS}
      ${QSODIUM_CXX}
)

target_include_directories(qt-sodium PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)

target_link_libraries(qt-sodium PRIVATE
    ${LibSodium_LIBRARIES}
    Qt6::Core
)
