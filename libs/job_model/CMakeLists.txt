project(JosephsOddBuilder_Model
    VERSION 0.1
    LANGUAGES CXX
)


set(JOBMODEL_HEADERS

    job_model_enums.h




    job_model.h # Facade
)
set(JOBMODEL_SOURCES
    job_model.cpp
)
# our first target is
set(JOBMODEL_CONFIG_HEADERS
    config/model_architecture.h
    config/model_config_concept.h
    config/arch_config.h
    config/sampler_config.h
    config/transformer_config.h
    config/moe_config.h
    config/attention_config.h
    config/output_head_config.h
    config/norm_config.h
    config/rope_config.h
    config/feed_forward_config.h
    config/device_config.h
    config/lora_config.h

    config/model_config.h
    config/arch/qwen/qwen3_instruct_2507.h   # refactor time
)
set(JOBMODEL_CONFIG_IO_HEADERS

)

set(JOBMODEL_CONFIG_SOURCES
    config/arch_config.cpp
    config/sampler_config.cpp
    config/moe_config.cpp
    config/attention_config.cpp
    config/output_head_config.cpp
    config/norm_config.cpp
    config/rope_config.cpp
    config/feed_forward_config.cpp
    config/transformer_config.cpp
    config/device_config.cpp
    config/lora_config.cpp

    config/model_config.cpp
)

set(JOBMODEL_CONFIG_IO_SOURCES
)


set(JOBMODEL_CTX_HEADERS
    ctx/job_mem_buf.h
    ctx/job_ai_ctx.h
)

set(JOBMODEL_CTX_SOURCES
    ctx/job_mem_buff.cpp
    ctx/job_ai_ctx.cpp
)

set(JOBMODEL_IO_HEADERS
    io/job_iio_reader.h
    io/job_iio_writer.h

    ## CONFIGS
    io/model_config_reader_utils.h
    io/imodel_config_reader.h
    io/gguf_model_config_reader.h
    io/hf_json_model_config_reader.h
    io/safetensors_model_config_reader.h
)

set(JOBMODEL_IO_SOURCES

    ## CONFIGS
    io/model_config_reader_utils.cpp
    io/imodel_config_reader.cpp
    io/gguf_model_config_reader.cpp
    io/hf_json_model_config_reader.cpp
    io/safetensors_model_config_reader.cpp
)


set(JOBMODEL_WEIGHTS_HEADERS
    weights/model_weights.h
)
set(JOBMODEL_WEIGHTS_SOURCES
    weights/model_weights.cpp
)

set(JOBMODEL_KV_HEADERS
    ## concepts
    kv/job_mem_concept.h


    kv/job_batch_data.h
    kv/job_ubatch.h
    kv/job_batch.h
    kv/job_imem_config.h  ## move to the other configs
    kv/job_imem_ctx.h
    kv/job_batch_alloc.h
    kv/job_imem.h
    kv/kv_cache.h
)

set(JOBMODEL_KV_SOURCES
    kv/job_imem_ctx.cpp
    kv/job_imem.cpp
    kv/kv_cache.cpp
)

set(JOBMODEL_GRAPH_HEADERS
    graph/linear_graph.h
    graph/norm_graph.h
    graph/rope_graph.h
    graph/gqa_graph.h
    graph/attention_graph.h
    graph/gated_ffn_graph.h
    graph/embedding_graph.h
    graph/residual_graph.h
    graph/graph_builder.h
    # graph/compute_graph_builder.h  Kick rocks bro
)

set(JOBMODEL_GRAPH_SOURCES
    graph/linear_graph.cpp
    graph/norm_graph.cpp
    graph/rope_graph.cpp
    graph/gqa_graph.cpp
    graph/attention_graph.cpp
    graph/gated_ffn_graph.cpp
    graph/embedding_graph.cpp
    graph/residual_graph.cpp
    graph/graph_builder.cpp
    # graph/compute_graph_builder.cpp  # Kick rocks bro
)


set(JOBMODEL_GRAPH_ARCH_HEADERS
    graph/arch/qwen_graph_builder.h
)

set(JOBMODEL_GRAPH_ARCH_SOURCES
    graph/arch/qwen_graph_builder.cpp
)

set(JOBMODEL_SAMPLE_HEADERS
    sample/sampler.h
)

set(JOBMODEL_SAMPLE_SOURCES
    sample/sampler.cpp
)

set(JOBMODEL_COMPILE_THIS
    ${JOBMODEL_HEADERS}             ${JOBMODEL_SOURCES}
    ${JOBMODEL_CONFIG_HEADERS}      ${JOBMODEL_CONFIG_SOURCES}
    ${JOBMODEL_IO_HEADERS}          ${JOBMODEL_IO_SOURCES}
    ${JOBMODEL_WEIGHTS_HEADERS}     ${JOBMODEL_WEIGHTS_SOURCES}
    ${JOBMODEL_KV_HEADERS}          ${JOBMODEL_KV_SOURCES}
    ${JOBMODEL_GRAPH_HEADERS}       ${JOBMODEL_GRAPH_SOURCES}
    ${JOBMODEL_GRAPH_ARCH_HEADERS}  ${JOBMODEL_GRAPH_ARCH_SOURCES}
    ${JOBMODEL_SAMPLE_HEADERS}      ${JOBMODEL_SAMPLE_SOURCES}
    ${JOBMODEL_CTX_HEADERS}         ${JOBMODEL_CTX_SOURCES}
)

if(BUILD_SHARED_LIBS)
    add_library(JosephsOddBuilder_Model SHARED ${JOBMODEL_COMPILE_THIS})
else()
    add_library(JosephsOddBuilder_Model STATIC ${JOBMODEL_COMPILE_THIS})
    target_compile_definitions(JosephsOddBuilder_Model PUBLIC JOBMODEL_STATIC_DEFINE)
endif()


set(JOBMODEL_EXPORT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/jobmodel_export.h")
generate_export_header(JosephsOddBuilder_Model
    BASE_NAME JOBMODEL
    EXPORT_FILE_NAME "${JOBMODEL_EXPORT_HEADER}"
)

set_target_properties(JosephsOddBuilder_Model PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobmodel"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(JosephsOddBuilder_Model PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobmodel>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/config>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/config>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/io>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/io>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/weights>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/weights>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kv>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/kv>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/graph>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/graph>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/graph/arch>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/graph/arch>


    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sample>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/sample>


    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ctx>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/ctx>

    ## export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(JosephsOddBuilder_Model PRIVATE
    JosephsOddBuilder_Core
    JosephsOddBuilder_Simd
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Ggml
    JosephsOddBuilder_Token
    Threads::Threads
    $<BUILD_INTERFACE:ggml>
)

if(JOB_LINUX)
    target_link_libraries(JosephsOddBuilder_Model PUBLIC atomic)
elseif(JOB_WINDOWS)
    target_compile_options(JosephsOddBuilder_Model PRIVATE /wd4251)
endif()


install(TARGETS JosephsOddBuilder_Model
    EXPORT JosephsOddBuilderModelTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobmodel
)

install(FILES ${JOBMODEL_HEADERS}              DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobmodel)
install(FILES ${JOBMODEL_EXPORT_HEADER}        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobmodel)
install(FILES ${JOBMODEL_CONFIG_HEADERS}       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/config)
install(FILES ${JOBMODEL_IO_HEADERS}           DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/io)
install(FILES ${JOBMODEL_WEIGHTS_HEADERS}      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/weights)
install(FILES ${JOBMODEL_KV_HEADERS}           DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/kv)
install(FILES ${JOBMODEL_GRAPH_HEADERS}        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/graph)
install(FILES ${JOBMODEL_GRAPH_ARCH_HEADERS}   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/graph/arch)
install(FILES ${JOBMODEL_SAMPLE_HEADERS}       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/sample)
install(FILES ${JOBMODEL_CTX_HEADERS}          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/ctx)

if(JOB_LINUX)
    set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobmodel.pc")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/jobmodel.pc.in"
        "${PKGCONFIG_FILE}"
        @ONLY
    )

    install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}" )
endif()

install(EXPORT JosephsOddBuilderModelTargets
    FILE JosephsOddBuilderModelTargets.cmake
    NAMESPACE JOB::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderModel"
)

