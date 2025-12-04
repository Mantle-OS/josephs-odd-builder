cmake_minimum_required(VERSION 3.23)
project(JosephsOddBuilderAi
    VERSION 0.1
    LANGUAGES CXX
)

################### START IMP,DECL ################################

#######################
# SUBSYSTEMS
########################

# adapters
set(JOBAI_ADAPTERS_HEADERS
    adapters/iadapter.h
    adapters/adapter_types.h
    adapters/fmm_adapter.h
    # adapters/bh_adapter.h a bunch more
)
set(JOBAI_ADAPTERS_SOURCES)

# coach's
set(JOBAI_COACH_HEADERS
    coach/icoach.h
    coach/coach_types.h     ## make list opf the types I want to support and then add them even if the decl is empty
    coach/coach_config.h
    coach/genetic_coach.h   ## after contract
    coach/es_coach.h        ## after contract
)
set(JOBAI_COACH_SOURCES)

# cords
set(JOBAI_CORDS_HEADERS
    cords/aligned_allocator.h  # done
    cords/extents.h             # needs review
    cords/view_iter.h           # needs review
    cords/view.h                # needs review
    cords/rank.h                # needs review
    ## Tensors
    cords/fiber.h               # needs review
    cords/matrix.h              # needs review
    cords/volume.h              # needs review
    cords/batch.h               # needs review
    # Shapes
    cords/layout.h        ## need to start this=
    cords/tile4.h          ## need to start this=
    cords/tile8.h       ## need to start this=
    cords/tile16.h      ## need to start this=
)
set(JOBAI_CORDS_SOURCES)

# layers
set(JOBAI_LAYERS_HEADERS
    layers/layer_types.h    ## make list opf the types I want to support and then add them even if the decl is empty
    layers/ilayer.h          # need to lock this down
    layers/dense.h           # not started
)
set(JOBAI_LAYERS_SOURCES)

# mixure of experts
set(JOBAI_MOE_HEADERS
    moe/imoe.h               ## need to start this
    moe/moe_types.h          ## make list opf the types I want to support and then add them even if the decl is empty
    moe/sparse_moe.h         # not started

    # moe/dense_moe.h
    # moe/conv_moe.h
    # moe/attn_moe.h
    # moe/controller_moe.h
    # ....
    # irouter.h
    # irouter_types.h
    # hash_router.h
    # rule_router.h
    # .....


    ## Load balencer ?

)
set(JOBAI_MOE_SOURCES)

#######################
# NOT SUBSYSTEMS !
########################

set(JOBAI_BASE_HEADERS
    base/smid_amd.h # immintrin.h wrapper
)
set(JOBAI_BASE_SOURCES)

# comp
set(JOBAI_COMP_HEADERS
    comp/rrelu_config.h         # needs review
    comp/gdn_params.h           # needs review
    comp/activation_types.h     # done
    comp/activation_math.h      # neeeds review
    comp/gemm.h                 # done
    comp/mlp.h                  # done
    comp/flash_attention.h      # done
)
set(JOBAI_COMP_SOURCES)

# evo
set(JOBAI_EVO_HEADERS
    evo/genome.h                # done
    evo/crossover.h             # not started
    evo/mutator.h               # not started
    evo/population.h            # not started
    evo/species.h               # not started
)
set(JOBAI_EVO_SOURCES
    evo/genome.cpp              # empty for now
)

# infer
set(JOBAI_INFER_HEADERS
    infer/runner.h              # way later
)
set(JOBAI_INFER_SOURCES
    infer/runner.cpp            # way later
)

# kv
#######################
set(JOBAI_KV_HEADERS
    kv/kv_cache.h               # not planned
)
set(JOBAI_KV_SOURCES)


################### END IMP,DECL ################################

add_compile_options(-mavx -mavx2 -mfma)

add_library(JosephsOddBuilder_Ai SHARED
    # modules
    ${JOBAI_ADAPTERS_HEADERS}   ${JOBAI_ADAPTERS_SOURCES}
    ${JOBAI_COACH_HEADERS}      ${JOBAI_COACH_SOURCES}
    ${JOBAI_CORDS_HEADERS}      ${JOBAI_CORDS_HEADERS}
    ${JOBAI_LAYERS_HEADERS}     ${JOBAI_LAYERS_SOURCES}
    ${JOBAI_MOE_HEADERS}        ${JOBAI_MOE_SOURCES}
    # non modules
    ${JOBAI_BASE_HEADERS}       ${JOBAI_BASE_SOURCES}
    ${JOBAI_COMP_HEADERS}       ${JOBAI_COMP_SOURCES}
    ${JOBAI_EVO_HEADERS}        ${JOBAI_EVO_SOURCES}
    ${JOBAI_INFER_HEADERS}      ${JOBAI_INFER_SOURCES}
    ${JOBAI_KV_HEADERS}         ${JOBAI_KV_SOURCES}
)

target_compile_options(JosephsOddBuilder_Ai PRIVATE -mavx -mavx2 -mfma -Wpsabi)

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
    JosephsOddBuilder_Science
)

target_include_directories(JosephsOddBuilder_Ai PUBLIC
    # BUILD PATHS
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/adapters>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/coach>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cords>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/layers>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/moe>
    ## non modules
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/base>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/comp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/evo>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/infer>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kv>
    # INSTALL PATHS
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/adapters>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/coach>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/cords>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/layers>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/moe>
    ## non modules
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/base>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/comp>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/evo>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/infer>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/kv>
)


## Module HEADERS install
install(FILES ${JOBAI_ADAPTERS_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/adapters)
install(FILES ${JOBAI_COACH_HEADERS}    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/coach)
install(FILES ${JOBAI_CORDS_HEADERS}    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/cords)
install(FILES ${JOBAI_LAYERS_HEADERS}   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/layers)
install(FILES ${JOBAI_MOE_HEADERS}      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/moe)

## NON Module HEADERS  install
install(FILES ${JOBAI_BASE_HEADERS}     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/base)
install(FILES ${JOBAI_COMP_HEADERS}     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/comp)
install(FILES ${JOBAI_EVO_HEADERS}      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/evo)
install(FILES ${JOBAI_INFER_HEADERS}    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/infer)
install(FILES ${JOBAI_KV_HEADERS}       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/kv)

## PKGCONFIG
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobai.pc")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/jobai.pc.in" "${PKGCONFIG_FILE}" @ONLY)
install(FILES "${PKGCONFIG_FILE}"       DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

## Library install
install(TARGETS JosephsOddBuilder_Ai
    EXPORT      JosephsOddBuilderAITargets
    LIBRARY     DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE     DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME     DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
## Export the Libraray
install(EXPORT JosephsOddBuilderAITargets
    FILE JosephsOddBuilderAITargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderAi
)

# CHECKPOINT: v1.6
