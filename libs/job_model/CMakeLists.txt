project(JosephsOddBuilder_Model
    VERSION 0.1
    LANGUAGES CXX
)


set(JOBMODEL_HEADERS
    job_model.h # Facade
)
set(JOBMODEL_SOURCES
    job_model.cpp
)
# our first target is
set(JOBMODEL_CONFIG_HEADERS
    config/model_architecture.h         # enums and string
    config/model_config_concept.h       # base conert
    config/arch_config.h                # base archedcture config
    config/sampler_config.h             # the samplers config if needed
    config/transformer_config.h         # transformer configurations
    config/model_config_reader.h        # read config from json or gguf if possible
    # config/model_config_writer.h        # write config from json or gguf if possible

    config/model_config.h               # the top level config that own the sampler and friends.
    config/arch/qwen/qwen3_instruct_2507.h   # the 1st target
)

set(JOBMODEL_CONFIG_SOURCES
    # config/transformer_config.cpp
    config/model_config_reader.cpp

)

set(JOBMODEL_WEIGHTS_HEADERS
    weights/model_weights.h
)
set(JOBMODEL_WEIGHTS_SOURCES
    weights/model_weights.cpp
)

set(JOBMODEL_KV_HEADERS
    kv/kv_cache.h
)
set(JOBMODEL_KV_SOURCES
    kv/kv_cache.cpp
)

set(JOBMODEL_GRAPH_HEADERS
    graph/compute_graph_builder.h
)
set(JOBMODEL_GRAPH_SOURCES
    graph/compute_graph_builder.cpp
)

set(JOBMODEL_SAMPLE_HEADERS
    sample/sampler.h
)
set(JOBMODEL_SAMPLE_SOURCES
    sample/sampler.cpp
)

set(JOBMODEL_COMPILE_THIS
    ${JOBMODEL_HEADERS}            ${JOBMODEL_SOURCES}
    ${JOBMODEL_CONFIG_HEADERS}     ${JOBMODEL_CONFIG_SOURCES}
    ${JOBMODEL_WEIGHTS_HEADERS}    ${JOBMODEL_WEIGHTS_SOURCES}
    ${JOBMODEL_KV_HEADERS}         ${JOBMODEL_KV_SOURCES}
    ${JOBMODEL_GRAPH_HEADERS}      ${JOBMODEL_GRAPH_SOURCES}
    ${JOBMODEL_SAMPLE_HEADERS}     ${JOBMODEL_SAMPLE_SOURCES}
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

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/weights>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/weights>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kv>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/kv>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/graph>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/graph>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sample>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/sample>

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
install(FILES ${JOBMODEL_WEIGHTS_HEADERS}      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/weights)
install(FILES ${JOBMODEL_KV_HEADERS}           DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/kv)
install(FILES ${JOBMODEL_GRAPH_HEADERS}        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobmodel/graph)
install(FILES ${JOBMODEL_SAMPLE_HEADERS}       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobmodeln/sample)

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

