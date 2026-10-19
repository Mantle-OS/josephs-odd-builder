project(JosephsOddBuilder_Ai_Tools
    VERSION 0.1
    LANGUAGES CXX
)


set(JOBAITOOLS_HEADERS

    job_ai_tools.h

)

set(JOBAITOOLS_SOURCES
    job_ai_tools.cpp
)


# // https://github.com/modelcontextprotocol/modelcontextprotocol/blob/main/schema/2026-07-28/schema.json
set(JOBAITOOLS_SCHEMA_HEADERS
)

set(JOBAITOOLS_SCHEMA_SOURCES

)

# // gonna need versions for the headers
# see job_net, job_io
set(JOBAITOOLS_IPC_HEADERS

)

set(JOBAITOOLS_IPC_SOURCES

)

set(JOBAITOOLS_CMD_HEADERS
)

set(JOBAITOOLS_CMD_SOURCES

)

set(JOBAITOOLS_COMPILE_THIS
    ${JOBAITOOLS_HEADERS}             ${JOBAITOOLS_SOURCES}
    ${JOBAITOOLS_SCHEMA_HEADERS}      ${JOBAITOOLS_SCHEMA_SOURCES}
    ${JOBAITOOLS_IPC_HEADERS}         ${JOBAITOOLS_IPC_SOURCES}
    ${JOBAITOOLS_CMD_HEADERS}         ${JOBAITOOLS_CMD_SOURCES}
)

if(BUILD_SHARED_LIBS)
    add_library(JosephsOddBuilder_Ai_Tools SHARED ${JOBAITOOLS_COMPILE_THIS})
else()
    add_library(JosephsOddBuilder_Ai_Tools STATIC ${JOBAITOOLS_COMPILE_THIS})
    target_compile_definitions(JosephsOddBuilder_Ai_Tools PUBLIC JOBAITOOLS_STATIC_DEFINE)
endif()


set(JOBAITOOLS_EXPORT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/jobaitools_export.h")
generate_export_header(JosephsOddBuilder_Ai_Tools
    BASE_NAME JOBAITOOLS
    EXPORT_FILE_NAME "${JOBMODEL_EXPORT_HEADER}"
)

# bring in things that need c++23 as a test and also just some randoms
target_compile_options(JosephsOddBuilder_Ai_Tools PRIVATE
    -freflection
    -fcontracts)

target_link_options(JosephsOddBuilder_Ai_Tools PRIVATE
    -fcontracts
)

set_target_properties(JosephsOddBuilder_Ai_Tools PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobaitools"
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(JosephsOddBuilder_Ai_Tools PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobaitools>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/schema>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobaitools/schema>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ipc>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobaitools/ipc>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cmd>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobaitools/cmd>

    ## export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(JosephsOddBuilder_Ai_Tools PRIVATE
    Threads::Threads
    atomic
    $<BUILD_INTERFACE:ggml>
    ${YAMLCpp_LIBRARIES}
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
    JosephsOddBuilder_IO
    JosephsOddBuilder_Net

)

install(TARGETS JosephsOddBuilder_Ai_Tools
    EXPORT JosephsOddBuilderAiToolsTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobaitools
)

install(FILES ${JOBMODEL_HEADERS}              DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobaitools)
install(FILES ${JOBAITOOLS_EXPORT_HEADER}      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobaitools)
install(FILES ${JOBMODEL_IPC_HEADERS}          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobaitools/ipc)
install(FILES ${JOBMODEL_SCHEMA_HEADERS}       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobaitools/schema)
install(FILES ${JOBMODEL_CMD_HEADERS}          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobaitools/cmd)


set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobaitools.pc")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobaitools.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}" )


install(EXPORT JosephsOddBuilderAiToolsTargets
    FILE JosephsOddBuilderAiToolsTargets.cmake
    NAMESPACE JOB::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderAiToolsTargets"
)

