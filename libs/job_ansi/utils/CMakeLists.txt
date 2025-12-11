set(TARGET_NAME ansipp-utils)
project(${TARGET_NAME}
    VERSION 0.1
    DESCRIPTION ""
    LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ANSI_UTILS_CXX
    color_utils.h
    calback_macros.h
    ansipp_enums.h
    ansipp_suffix.h
    rgb_color.h
    utf8_decoder.h      utf8_decoder.cpp
    ansipp_utils.h      ansipp_utils.cpp
)

add_library(${TARGET_NAME} STATIC ${ANSI_UTILS_CXX} )
target_include_directories(${TARGET_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)
set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 23)

