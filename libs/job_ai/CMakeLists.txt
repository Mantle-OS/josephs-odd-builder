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
    adapters/adapter_ctx.h
    adapters/iadapter.h
    adapters/adapter_types.h
    # physics style adapter's
    adapters/fmm_config.h
    adapters/fmm_adapter.h
    adapters/bh_config.h
    adapters/bh_adapter.h
    adapters/verlet_config.h
    adapters/verlet_adapter.h
    # LLM style adapter's
    adapters/flash_adapter.h
    adapters/lowrank_config.h
    adapters/lowrank_adapter.h
    adapters/dense_config.h
    adapters/dense_adapter.h
    ## Factory floor
    adapters/adapter_factory.h
)
set(JOBAI_ADAPTERS_SOURCES
    adapters/fmm_adapter.cpp
    adapters/bh_adapter.cpp
    adapters/verlet_adapter.cpp
    adapters/dense_adapter.cpp
    adapters/lowrank_adapter.cpp
    adapters/flash_adapter.cpp
)

# coach's
set(JOBAI_COACH_HEADERS
    coach/icoach.h              # done needs love
    coach/coach_types.h         # done'ish
    coach/coach_config.h        # started and looks nice
    coach/genetic_coach.h       ## after contract
    coach/es_coach.h            ## working example needs perf
    coach/fitness_evaluator.h
    coach/portal_evaluator.h

)
set(JOBAI_COACH_SOURCES)

# cords
set(JOBAI_CORDS_HEADERS
    cords/attention_shape.h
    cords/aligned_allocator.h   # done
    cords/extents.h             # done
    cords/view_iter.h           # done
    cords/view.h                # done
    cords/rank.h                # done
    ## Tensors
    cords/fiber.h               # done
    cords/matrix.h              # done
    cords/volume.h              # done
    cords/batch.h               # done
    # Shapes
    cords/layout.h              ## done
    cords/tile4.h               ## done
    cords/tile8.h               ## done
    cords/tile16.h              ## done
)
set(JOBAI_CORDS_SOURCES)

# layers
set(JOBAI_LAYERS_HEADERS
    layers/layer_types.h
    layers/paramgroup_type.h    # new
    layers/paramgroup_config.h  # new
    layers/iparamgroup.h        # << we are here

    layers/ilayer.h

    layers/attention_config.h
    layers/attention.h
    layers/dense.h
    layers/layer_factory.h
    # layers/layer_lora.h

)
set(JOBAI_LAYERS_SOURCES
    layers/layer_factory.cpp
)

# mixure of experts
set(JOBAI_MOE_HEADERS
    moe/imoe.h                  ## done'sh
    moe/moe_types.h             ## done'ish
    moe/sparse_moe.h            # needs refactor

    # moe/dense_moe.h
    # moe/conv_moe.h
    # moe/attn_moe.h
    # moe/controller_moe.h
    # ....
    # .....


    ## Load balencer ?

)
set(JOBAI_MOE_SOURCES)

set(JOBAI_ROUTER_HEADERS
    # router/irouter.h
    router/router_expert.h
    router/router_configs.h
    router/router_plan.h
    router/router_types.h

    router/router_impl.h

)
set(JOBAI_ROUTER_SOURCES
    router/router_impl.cpp
)

#######################
# NOT SUBSYSTEMS !
########################

set(JOBAI_BASE_HEADERS
)
set(JOBAI_BASE_SOURCES)

# comp
set(JOBAI_COMP_HEADERS
    comp/rounding_mode.h
    comp/noise_table.h
    comp/simd_provider.h
    comp/transpose.h
    comp/activation_functors.h
    comp/activate_parallel.h
    #
    comp/rrelu_config.h         # done'ish
    comp/gdn_params.h           # done'ish
    comp/activation_types.h     # done
    comp/activation_math.h      # done'ish
    comp/gemm.h                 # done
    comp/mlp.h                  # done
    comp/flash_attention.h      # done
    comp/atomic_math.h          # done
)
set(JOBAI_COMP_SOURCES)

if(HAS_AVX)
    list(APPEND  JOBAI_COMP_HEADERS comp/simd_avx.h)
elseif(HAS_NEON)
    list(APPEND  JOBAI_COMP_HEADERS comp/simd_arm.h)
endif()


# evo
set(JOBAI_EVO_HEADERS
    evo/genome.h                # done
    evo/individual.h            # might not be needed
    evo/crossover_type.h
    evo/crossover_config.h
    evo/crossover.h
    evo/mutator_config.h
    evo/mutator.h
    evo/population_config.h
    evo/population.h
    evo/species_config.h
    evo/species.h
    evo/speciation_engine.h
)
set(JOBAI_EVO_SOURCES
    evo/genome.cpp              # empty for now
)

# infer
set(JOBAI_INFER_HEADERS
    infer/workspace.h
    infer/runner.h              # way later
)
set(JOBAI_INFER_SOURCES
    infer/runner.cpp            # way later
)

# cache
set(JOBAI_CACHE_HEADERS
    cache/kv_key.h
    cache/kv_cache.h               # not planned
)
set(JOBAI_CACHE_SOURCES)


# learner
set(JOBAI_LEARNER_HEADERS
    learner/learn_concept.h
    learner/learn_type.h
    learner/learn_xor.h
    learner/learn_cart_pole.h
)
set(JOBAI_LEARNER_SOURCES)


################### END IMP,DECL ################################

add_compile_options(-mavx -mavx2 -mfma)

add_library(JosephsOddBuilder_Ai SHARED
    # modules
    ${JOBAI_ADAPTERS_HEADERS}   ${JOBAI_ADAPTERS_SOURCES}
    ${JOBAI_COACH_HEADERS}      ${JOBAI_COACH_SOURCES}
    ${JOBAI_CORDS_HEADERS}      ${JOBAI_CORDS_HEADERS}
    ${JOBAI_LAYERS_HEADERS}     ${JOBAI_LAYERS_SOURCES}
    ${JOBAI_LEARNER_HEADERS}    ${JOBAI_LEARNER_SOURCES}
    ${JOBAI_MOE_HEADERS}        ${JOBAI_MOE_SOURCES}
    ${JOBAI_ROUTER_HEADERS}     ${JOBAI_ROUTER_SOURCES}
    # non modules
    # ${JOBAI_BASE_HEADERS}       ${JOBAI_BASE_SOURCES}
    ${JOBAI_COMP_HEADERS}       ${JOBAI_COMP_SOURCES}
    ${JOBAI_EVO_HEADERS}        ${JOBAI_EVO_SOURCES}
    ${JOBAI_INFER_HEADERS}      ${JOBAI_INFER_SOURCES}
    ${JOBAI_CACHE_HEADERS}      ${JOBAI_CACHE_SOURCES}
)

target_compile_options(JosephsOddBuilder_Ai PRIVATE ${JOB_CXX_FLAGS})

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
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/adapters>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/coach>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cords>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/layers>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/learner>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/moe>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/router>
    ## non modules
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/base>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/comp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/evo>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/infer>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cache>
    # INSTALL PATHS
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/adapters>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/coach>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/cords>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/layers>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/learner>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/moe>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/router>
    ## non modules
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/base>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/comp>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/evo>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/infer>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/cache>
)


## Module HEADERS install
install(FILES ${JOBAI_ADAPTERS_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/adapters)
install(FILES ${JOBAI_COACH_HEADERS}    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/coach)
install(FILES ${JOBAI_CORDS_HEADERS}    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/cords)
install(FILES ${JOBAI_LAYERS_HEADERS}   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/layers)
install(FILES ${JOBAI_LEARNER_HEADERS}   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/learner)
install(FILES ${JOBAI_MOE_HEADERS}      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/moe)
install(FILES ${JOBAI_MOE_HEADERS}      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/router)

## NON Module HEADERS  install
install(FILES ${JOBAI_BASE_HEADERS}     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/base)
install(FILES ${JOBAI_COMP_HEADERS}     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/comp)
install(FILES ${JOBAI_EVO_HEADERS}      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/evo)
install(FILES ${JOBAI_INFER_HEADERS}    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/infer)
install(FILES ${JOBAI_CACHE_HEADERS}    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/cache)

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
