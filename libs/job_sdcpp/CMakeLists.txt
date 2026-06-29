cmake_minimum_required(VERSION 3.16)
project(JosephsOddBuilder_SdCpp
    VERSION 0.1
    LANGUAGES CXX
)

set(JOBSDCPP_HEADERS
)

set(JOBSDCPP_SOURCES
)

add_library(JosephsOddBuilder_SdCpp SHARED
    ${JOBSDCPP_HEADERS}
    ${JOBSDCPP_SOURCES}
)

set_target_properties(JosephsOddBuilder_SdCpp PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobsdcpp"
)

target_include_directories(JosephsOddBuilder_SdCpp PUBLIC
    ${LibGgml_INCLUDE_DIRS}
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobsdcpp>
)

target_link_libraries(JosephsOddBuilder_SdCpp PUBLIC
    Threads::Threads
    atomic
    ${LibGgml_LIBRARIES}
    ${LibGgmlBase}
    JosephsOddBuilder_Core
)

install(TARGETS JosephsOddBuilder_SdCpp
    EXPORT      JosephsOddBuilderSdCppTargets
    LIBRARY     DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE     DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME     DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${JOBSDCPP_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobsdcpp)

## PKGCONFIG
set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobsdcpp.pc")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/jobsdcpp.pc.in" "${PKGCONFIG_FILE}" @ONLY)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderSdCppTargets
    FILE JosephsOddBuilderSdCppTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderSdCpp
)
