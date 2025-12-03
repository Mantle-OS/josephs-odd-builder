cmake_minimum_required(VERSION 3.16)
project(JosephsOddBuilderScience
    VERSION 0.1
    LANGUAGES CXX
)

# JOB_SCIENCE_USE_DOUBLE

option(JOB_SCIENCE_USE_MMAP "Enable memory-mapped frame reading" OFF)
if(JOB_SCIENCE_USE_MMAP)
    add_compile_definitions(JOB_SCIENCE_USE_MMAP)
endif()


option(JOB_SCIENCE_USE_DOUBLE "Use double precision" OFF)
if(JOB_SCIENCE_USE_DOUBLE)
    add_compile_definitions(JOB_SCIENCE_USE_DOUBLE)
endif()

set(JOB_SCIENCE_DATA_HEADERS
    data/vec3f.h
    data/real_type.h
    data/composition.h
    data/particle.h
    data/disk.h
    data/forces.h
    data/zones.h
)

set(JOB_SCIENCE_DATA_SOURCES
    data/forces.cpp
    data/zones.cpp
    data/particle.cpp
    data/disk.cpp
    data/composition.cpp
)

set(JOB_SCIENCE_FRAMES_HEADERS
    frames/endian_utils.h
    frames/crc32.h
    frames/frame_header.h
    # frames/frame_replay.h

    frames/iframe_sink.h
    frames/frame_sink_io.h
    frames/frame_sink_net.h
    frames/frame_serializer.h

    frames/iframe_source.h
    frames/frame_source_io.h
    frames/frame_source_net.h
    frames/frame_deserializer.h


)
set(JOB_SCIENCE_FRAMES_SOURCES
    frames/crc32.cpp
    frames/frame_header.cpp #// DEAD

    frames/frame_sink_io.cpp
    frames/frame_sink_net.cpp
    frames/frame_serializer.cpp

    frames/frame_source_io.cpp
    frames/frame_source_net.cpp
    frames/frame_deserializer.cpp

    # frames/frame_replay.cpp
)


## MOVE ALL THE THINGSD FROM THRREADS TO HERE
set(JOB_SCIENCE_ENGINE_SOURCES
    engine/engine.cpp
)
set(JOB_SCIENCE_ENGINE_HEADERS
    engine/engine_config.h
    engine/engine.h
    engine/integrator_strategy.h
)

set(JOB_SCIENCE_SOLVERS_SOURCES
    solvers/bnh.cpp
    solvers/dijkstra.cpp
    solvers/euler.cpp
    solvers/factory.cpp
    solvers/fmm.cpp
    solvers/rk4.cpp
    solvers/vv.cpp
)

set(JOB_SCIENCE_SOLVERS_HEADERS
    solvers/bnh.h
    solvers/dijkstra.h
    solvers/euler.h
    solvers/factory.h
    solvers/fmm.h
    solvers/rk4.h
    solvers/vv.h
)

add_library(JosephsOddBuilder_Science SHARED
    ${JOB_SCIENCE_DATA_HEADERS}        ${JOB_SCIENCE_DATA_SOURCES}
    ${JOB_SCIENCE_FRAMES_HEADERS}      ${JOB_SCIENCE_FRAMES_SOURCES}
    ${JOB_SCIENCE_ENGINE_HEADERS}      ${JOB_SCIENCE_ENGINE_SOURCES}
    ${JOB_SCIENCE_INTEGRATORS_HEADERS} ${JOB_SCIENCE_INTEGRATORS_SOURCES}
 )

set_target_properties(JosephsOddBuilder_Science PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobscience"
)

target_link_libraries(JosephsOddBuilder_Science PUBLIC
    Threads::Threads
    atomic
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Net
    JosephsOddBuilder_Crypto
)
target_include_directories(JosephsOddBuilder_Science PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/frames>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/data>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/integrators>
    ## install paths
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobscience/engine>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobscience/frames>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobscience/data>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobscience/integrators>
)

install(TARGETS JosephsOddBuilder_Science
    EXPORT JosephsOddBuilderScienceTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOB_SCIENCE_DATA_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobscience/data
)

install(FILES ${JOB_SCIENCE_FRAMES_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobscience/frames
)

install(FILES ${JOB_SCIENCE_ENGINE_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobscience/engine
)

install(FILES ${JOB_SCIENCE_INTEGRATORS_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobscience/integrators
)

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobscience.pc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobscience.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderScienceTargets
    FILE JosephsOddBuilderScienceTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderScience
)
