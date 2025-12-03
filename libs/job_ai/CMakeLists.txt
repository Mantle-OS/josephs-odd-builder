cmake_minimum_required(VERSION 3.23)
project(JosephsOddBuilderAi
    VERSION 0.1
    LANGUAGES CXX
)

############################
# Base
############################
set(JOBAI_BASE_HEADERS
    base/activation_types.h
    base/layer_types.h
    base/aligned_allocator.h
    base/activation_math.h
    base/rrelu_config.h
    base/gdn_params.h
    base/genome.h
    base/activation.h

    base/tensor_shape.h
    base/tensor_rank.h
    base/tensor_iter.h
    base/tensor_view.h
    base/tensor_1d.h
    base/tensor_2d.h
    base/tensor_3d.h
    base/tensor_4d.h
)
set(JOBAI_BASE_SOURCES
    base/genome.cpp
)

#######################
# Coach
########################
set(JOBAI_COACH_HEADERS
    coach/es_trainer.h
    coach/genetic_trainer.h
    coach/icoach.h
)
set(JOBAI_COACH_SOURCES

)

#######################
# Evo
########################
set(JOBAI_EVO_HEADERS
    evo/crossover.h
    evo/mutator.h
    evo/population.h
    evo/species.h
)
set(JOBAI_EVO_SOURCES

)

#######################
# Infer
########################
set(JOBAI_INFER_HEADERS
    infer/runner.h
)
set(JOBAI_INFER_SOURCES
    infer/runner.cpp
)

#######################
# Machine
#######################
set(JOBAI_MACHINE_HEADERS
    machine/dense.h
    machine/ilayer.h
    machine/sparse_moe.h
)
set(JOBAI_MACHINE_SOURCES

)

#######################
# Att
#######################
set(JOBAI_ATT_HEADERS
    machine/att/fmm_adapter.h
    machine/att/kv_cache.h
)
set(JOBAI_ATT_SOURCES

)

add_library(JosephsOddBuilder_Ai SHARED
    ${JOBAI_BASE_HEADERS}       ${JOBAI_BASE_SOURCES}
    ${JOBAI_COACH_HEADERS}      ${JOBAI_COACH_SOURCES}
    ${JOBAI_EVO_HEADERS}        ${JOBAI_EVO_SOURCES}
    ${JOBAI_INFER_HEADERS}      ${JOBAI_INFER_SOURCES}
    ${JOBAI_MACHINE_HEADERS}    ${JOBAI_MCHINE_SOURCES}
    ${JOBAI_ATT_HEADERS}        ${JOBAI_ATT_SOURCES}
)

set_target_properties(JosephsOddBuilder_Ai PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobai"
)

target_link_libraries(JosephsOddBuilder_Ai PUBLIC
    Threads::Threads
    atomic
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Crypto
    JosephsOddBuilder_Net
)

target_include_directories(JosephsOddBuilder_Ai PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/base>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/coach>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/evo>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/infer>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/machine>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/machine/att>
    ## install paths
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/base>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/coach>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/evo>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/infer>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/machine>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/machine/att>
)

install(TARGETS JosephsOddBuilder_Ai
    EXPORT JosephsOddBuilderAITargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBAI_BASE_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/base
)

install(FILES ${JOBAI_COACH_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/coach
)

install(FILES ${JOBAI_EVO_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/evo
)

install(FILES ${JOBAI_INFER_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/infer
)

install(FILES ${JOBAI_MAVHINE_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/machine
)

install(FILES ${JOBAI_ATT_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/machine/att
)

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobai.pc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobai.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderAITargets
    FILE JosephsOddBuilderAITargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderAi
)

# CHECKPOINT: v1.5
