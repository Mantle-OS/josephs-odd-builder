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
    adapters/adapter_types.h
    adapters/iadapter.h
    # physics style adapter's
    adapters/fmm_config.h
    adapters/fmm_adapter.h
    adapters/bh_config.h
    adapters/bh_adapter.h
    adapters/verlet_config.h
    adapters/verlet_adapter.h
    adapters/stencil_config.h
    adapters/stencil_adapter.h
    adapters/rk4_config.h
    adapters/rk4_adapter.h
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
    adapters/stencil_adapter.cpp
    adapters/rk4_adapter.cpp
)

# coach's
set(JOBAI_COACH_HEADERS
    coach/coach_types.h
    coach/icoach.h
    coach/coach_config.h
    coach/genetic_coach.h
    coach/es_coach.h
)
set(JOBAI_COACH_SOURCES)

# cords
set(JOBAI_CORDS_HEADERS
    cords/attention_shape.h
    cords/aligned_allocator.h
    cords/extents.h
    cords/view_iter.h
    cords/view.h
    cords/rank.h
    ## Tensors
    cords/fiber.h
    cords/matrix.h
    cords/volume.h
    cords/batch.h
    # Shapes
    cords/layout.h
    cords/tile4.h
    cords/tile8.h
    cords/tile16.h
    cords/ml_particle.h
)
set(JOBAI_CORDS_SOURCES)

# layers
set(JOBAI_LAYERS_HEADERS
    layers/layer_types.h
    layers/layer_config.h

    layers/paramgroup_type.h
    layers/paramgroup_config.h
    layers/iparamgroup.h

    layers/ilayer.h
    layers/abstract_layer.h

    layers/attention_config.h
    layers/attention.h
    layers/dense.h
    layers/layer_factory.h
    # layers/layer_lora.h # coming soon near some frozen weights near you

)
set(JOBAI_LAYERS_SOURCES
    layers/layer_factory.cpp
)


# mixure of experts
set(JOBAI_MOE_HEADERS
    moe/imoe.h
    moe/moe_types.h
    moe/sparse_moe.h

    # moe/dense_moe.h
    # moe/conv_moe.h
    # moe/attn_moe.h
    # moe/controller_moe.h
    # ....
    # .....


    ## Load balencer Later
)
set(JOBAI_MOE_SOURCES)

set(JOBAI_ROUTER_HEADERS
    # router/irouter.h
    router/router_types.h
    router/router_expert.h
    router/router_configs.h
    router/router_plan.h
    router/router_impl.h

)
set(JOBAI_ROUTER_SOURCES
    router/router_impl.cpp
)


# learner
set(JOBAI_LEARNER_HEADERS
    learner/learn_concept.h
    learner/ilearn.h
    learner/learn_config.h
    learner/learn_type.h
    learner/learn_xor.h
    learner/learn_cart_pole.h
    learner/learn_bard.h
    learner/learn_factory.h
)
set(JOBAI_LEARNER_SOURCES)



set(JOBAI_TOKEN_HEADERS
    token/byte_lattice_kernel.h ## Empty
    token/byte_lattice.h        ## Donish

    token/unicode_lattice_kernel.h  ## just finished
    token/unicode_lattice.h     # the Unicode lattice

    token/token_type.h          # enums of the token type plugins

    token/token_concept.h
    token/itoken.h              # the interface refactor slash donish

    token/token_safety_dance.h

    token/byte_token.h
    token/ascii_token.h         # refactor slash donish
    token/char_token.h          # the Gen 0-50 (refactor slash donish)
    token/corpus_chemist.h      # refactor slash donish
    token/motif_token.h         # Gen 50+: (refactor slash donish)
    # token/bpe_token.h           # compare against me EMPTY
    token/token_factory.h       # the facory go get aq uniq pointer of the plugin type refactor slash donish
)
set(JOBAI_TOKEN_SOURCES)

#######################
# NOT SUBSYSTEMS !
########################

set(JOBAI_BASE_HEADERS
)
set(JOBAI_BASE_SOURCES)

# comp


set(JOBAI_COMP_HEADERS
    comp/noise_table.h
    comp/atomic_math.h

    #
    comp/gemm.h
    comp/mlp.h
    comp/flash_attention.h

    #
    comp/gdn_params.h
    comp/rrelu_config.h


    comp/activation_types.h
    comp/elu.h
    comp/gelu.h
    comp/identity.h
    comp/mish.h
    comp/relu.h
    comp/sigmoid.h
    comp/softplus.h
    comp/special.h
    comp/swish.h
    comp/tanh.h

    comp/activation.h

    ## OLD
    # comp/iactivation.h
    # comp/fast_math.h
    # comp/activation_math.h
    # comp/activation_functors.h
)
set(JOBAI_COMP_SOURCES)



# evo
set(JOBAI_EVO_HEADERS
    evo/genome.h
    evo/individual.h
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
    evo/genome.cpp
)

# infer
set(JOBAI_INFER_HEADERS
    infer/workspace.h
    infer/workspace_arena.h
    infer/workspace_view.h
    infer/runner.h
)
set(JOBAI_INFER_SOURCES
    infer/runner.cpp
)

# cache
set(JOBAI_CACHE_HEADERS
    cache/kv_key.h
    cache/kv_cache.h
)
set(JOBAI_CACHE_SOURCES)




################### END IMP,DECL ################################

add_library(JosephsOddBuilder_Ai SHARED
    # modules
    ${JOBAI_ADAPTERS_HEADERS}   ${JOBAI_ADAPTERS_SOURCES}
    ${JOBAI_COACH_HEADERS}      ${JOBAI_COACH_SOURCES}
    ${JOBAI_CORDS_HEADERS}      ${JOBAI_CORDS_SOURCES}
    ${JOBAI_LAYERS_HEADERS}     ${JOBAI_LAYERS_SOURCES}
    ${JOBAI_LEARNER_HEADERS}    ${JOBAI_LEARNER_SOURCES}
    ${JOBAI_MOE_HEADERS}        ${JOBAI_MOE_SOURCES}
    ${JOBAI_ROUTER_HEADERS}     ${JOBAI_ROUTER_SOURCES}
    ${JOBAI_TOKEN_HEADERS}      ${JOBAI_TOKEN_SOURCES}
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
    JosephsOddBuilder_Ansi
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Simd
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
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/token>
    ## non modules
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/base>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/comp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/evo>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/infer>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cache>
    # INSTALL PATHS
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/adapters>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/coach>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/cords>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/layers>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/learner>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/moe>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/router>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobai/token>
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
install(FILES ${JOBAI_LEARNER_HEADERS}  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/learner)
install(FILES ${JOBAI_MOE_HEADERS}      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/moe)
install(FILES ${JOBAI_ROUTER_HEADERS}   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/router)
install(FILES ${JOBAI_TOKEN_HEADERS}    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobai/token)


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
