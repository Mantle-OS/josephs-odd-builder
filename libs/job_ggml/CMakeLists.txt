cmake_minimum_required(VERSION 3.16)
project(JosephsOddBuilder_Ggml
    VERSION 0.1
    LANGUAGES CXX
)


# [  4 bytes  ][ 4 bytes ][8 bytes][   8 bytes  ][      KV    ][     tensor info  ][      rest of file     ]

set(JOBGGML_HEADERS
    # job_gguf_val.h                  ## util value
    # job_gguf_header.h               ## 1st 24 bytes and 8 padding                       owned by job_ggml_model
    # job_gguf_abstract_kv.h          ## virtual kv                                       owned by job_ggml_model
    # job_gguf_abstract_tensor_info.h ## virtual tensor information                       owned by job_ggml_model
    job_ggml_weights.h              ## tensor storage.                                  owned by job_ggml_model
    job_ggml_graph.h                ## manage the graph and ctx.                        owned by job_ggml_model
    job_ggml_device.h               ## a single backend(sched is owned by manager).     owned by job_ggml_device_manager
    job_ggml_device_manager.h       ## manager of all backends                          owned by job_ggml_model
    job_ggml_model.h                ## god class                                        owned by application
)
set(JOBGGML_SOURCES
    job_ggml_device.cpp
    job_ggml_device_manager.cpp
    job_ggml_graph.cpp
    job_ggml_weights.cpp
    job_ggml_model.cpp
)

add_library(JosephsOddBuilder_Ggml SHARED
    ${JOBGGML_HEADERS}
    ${JOBGGML_SOURCES}
)

set_target_properties(JosephsOddBuilder_Ggml PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobggml"
)

target_include_directories(JosephsOddBuilder_Ggml PUBLIC
    ${LibGgml_INCLUDE_DIRS}
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobggml>
)

target_link_libraries(JosephsOddBuilder_Ggml PUBLIC
    Threads::Threads
    atomic
    ${LibGgml_LIBRARIES}
    ${LibGgmlBase}
    JosephsOddBuilder_Core
)

install(TARGETS JosephsOddBuilder_Ggml
    EXPORT      JosephsOddBuilderGgmlTargets
    LIBRARY     DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE     DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME     DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${JOBGGML_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobggml)

## PKGCONFIG
set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobggml.pc")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/jobggml.pc.in" "${PKGCONFIG_FILE}" @ONLY)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderGgmlTargets
    FILE JosephsOddBuilderGgmlTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderGgml
)
