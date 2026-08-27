cmake_minimum_required(VERSION 3.16)

project(JosephsOddBuilder_Ggml
    VERSION 0.1
    LANGUAGES CXX
)

set(JOBGGML_DEVICE_IMPL_HEADERS)
set(JOBGGML_DEVICE_IMPL_SOURCES)

if(GGML_CPU)
    list(APPEND JOBGGML_DEVICE_IMPL_HEADERS device/impl/job_ggml_cpu.h)
    list(APPEND JOBGGML_DEVICE_IMPL_SOURCES device/impl/job_ggml_cpu.cpp)
endif()

if(GGML_VULKAN)
    add_compile_definitions(JOB_GGML_VULKAN)
    list(APPEND JOBGGML_DEVICE_IMPL_HEADERS device/impl/job_ggml_vulkan.h)
    list(APPEND JOBGGML_DEVICE_IMPL_SOURCES device/impl/job_ggml_vulkan.cpp)
endif()

if(GGML_CUDA)
    add_compile_definitions(JOB_GGML_CUDA)
    list(APPEND JOBGGML_DEVICE_IMPL_HEADERS device/impl/job_ggml_cuda.h)
    list(APPEND JOBGGML_DEVICE_IMPL_SOURCES device/impl/job_ggml_cuda.cpp)
endif()

if(GGML_OPENCL)
    add_compile_definitions(JOB_GGML_OPENCL)
    list(APPEND JOBGGML_DEVICE_IMPL_HEADERS device/impl/job_ggml_opencl.h)
    list(APPEND JOBGGML_DEVICE_IMPL_SOURCES device/impl/job_ggml_opencl.cpp)
endif()

if(GGML_BLAS)
    add_compile_definitions(JOB_GGML_BLAS)
    list(APPEND JOBGGML_DEVICE_IMPL_HEADERS device/impl/job_ggml_blas.h)
    list(APPEND JOBGGML_DEVICE_IMPL_SOURCES device/impl/job_ggml_blas.cpp)
endif()

## Next on the chopping block
if(GGML_HEXAGON)
    add_compile_definitions(JOB_GGML_HEXAGON)
    list(APPEND JOBGGML_DEVICE_IMPL_HEADERS device/impl/job_ggml_hexagon.h)
    list(APPEND JOBGGML_DEVICE_IMPL_SOURCES device/impl/job_ggml_hexagon.cpp)
endif()

if(GGML_OPENVINO)
    add_compile_definitions(JOB_GGML_OPENVINO)
    list(APPEND JOBGGML_DEVICE_IMPL_HEADERS device/impl/job_ggml_openvino.h)
    list(APPEND JOBGGML_DEVICE_IMPL_SOURCES device/impl/job_ggml_openvino.cpp)
endif()

if(GGML_SYCL)
    # ggml-sycl.cpp:5163:    return GGML_BACKEND_DEVICE_TYPE_GPU;
    add_compile_definitions(JOB_GGML_SYCL)
    list(APPEND JOBGGML_DEVICE_IMPL_HEADERS device/impl/job_ggml_sycl.h)
    list(APPEND JOBGGML_DEVICE_IMPL_SOURCES device/impl/job_ggml_sycl.cpp)
endif()

if(GGML_WEBGPU)
    # ggml-webgpu.cpp:3754:    return GGML_BACKEND_DEVICE_TYPE_GPU;
    add_compile_definitions(JOB_GGML_WEBGPU)
    list(APPEND JOBGGML_DEVICE_IMPL_HEADERS device/impl/job_ggml_webgpu.h)
    list(APPEND JOBGGML_DEVICE_IMPL_SOURCES device/impl/job_ggml_webgpu.cpp)
endif()

if(GGML_ZDNN)
    # ggml-zdnn.cpp:476:    return GGML_BACKEND_DEVICE_TYPE_ACCEL;
    add_compile_definitions(JOB_GGML_ZDNN)
    list(APPEND JOBGGML_DEVICE_IMPL_HEADERS device/impl/job_ggml_zdnn.h)
    list(APPEND JOBGGML_DEVICE_IMPL_SOURCES device/impl/job_ggml_zdnn.cpp)
endif()

## Maybe later

# if(GGML_VIRTGPU)
#     add_compile_definitions(JOB_GGML_VIRTGPU)
#     list(APPEND JOBGGML_DEVICE_IMPL_HEADERS device/impl/job_ggml_virtgpu.h)
#     list(APPEND JOBGGML_DEVICE_IMPL_SOURCES device/impl/job_ggml_virtgpu.cpp)
# endif()

# if(GGML_METAL)
#     add_compile_definitions(JOB_GGML_METAL)
#     list(APPEND JOBGGML_DEVICE_IMPL_HEADERS device/impl/job_ggml_metal.h)
#     list(APPEND JOBGGML_DEVICE_IMPL_SOURCES device/impl/job_ggml_metal.cpp)
# endif()

# if(GGML_ZENDNN)
#     add_compile_definitions(JOB_GGML_ZENDNN)
#     list(APPEND JOBGGML_DEVICE_IMPL_HEADERS device/impl/job_ggml_zendnn.h)
#     list(APPEND JOBGGML_DEVICE_IMPL_SOURCES device/impl/job_ggml_zendnn.cpp)
# endif()

# if(GGML_CANN)
#     add_compile_definitions(JOB_GGML_CANN)
#     list(APPEND JOBGGML_DEVICE_IMPL_HEADERS device/impl/job_ggml_cann.h)
#     list(APPEND JOBGGML_DEVICE_IMPL_SOURCES device/impl/job_ggml_cann.cpp)
# endif()

# message(STATUS "JOBGGML Device Impl Headers: ${JOBGGML_DEVICE_IMPL_HEADERS}")
# message(FATAL_ERROR "JOBGGML Device Impl Sources: ${JOBGGML_DEVICE_IMPL_SOURCES}")

# Done
set(JOBGGML_DEVICE_HEADERS
    device/job_ggml_device_caps.h
    device/job_ggml_device_props.h
    device/job_ggml_device_interface.h
    device/job_ggml_device.h
    device/job_ggml_device_manager.h
)

# done
set(JOBGGML_BACKEND_HEADERS
    backend/job_ggml_backend_reg.h
    backend/job_ggml_backend_buffer_type.h
    backend/job_ggml_backend_buffer.h
    backend/job_ggml_backend_event.h
    backend/job_ggml_backend_buffer_view.h
    backend/job_ggml_backend.h
    backend/job_ggml_backend_sched.h
    backend/job_ggml_backend_graph_copy.h
    backend/job_ggml_backend_graph_plan.h
)

# done
set(JOBGGML_CTX_HEADERS
    ctx/job_ggml_init_params.h
    ctx/job_ggml_context_view.h
    ctx/job_ggml_context.h
    ctx/job_ggml_cgraph.h
)

# done
set(JOBGGML_TENSOR_HEADERS
    tensor/job_ggml_tensor_extents.h
    tensor/job_ggml_tensor_view.h
    tensor/job_ggml_tensor_operation.h
    tensor/job_ggml_tensor_data.h
    tensor/job_ggml_tensor_rank.h
    tensor/job_ggml_tensor_fiber.h
    tensor/job_ggml_tensor_matrix.h
    tensor/job_ggml_tensor_volume.h
    tensor/job_ggml_tensor_batch.h
    tensor/job_ggml_tensor_shapes.h
    tensor/job_ggml_tensor_layout.h
    tensor/job_ggml_tensor.h
    tensor/job_ggml_tensor_op.h
    tensor/job_ggml_tensor_op_graph.h
)

# done
set(JOBGGML_ALLOC_HEADERS
    alloc/job_ggml_tensor_allocator.h
    alloc/job_ggml_graph_allocator.h
)

# done
set(JOBGGML_GGUF_HEADERS
    gguf/job_gguf_init_params.h
    gguf/job_gguf_tensor_info.h
    gguf/job_gguf_type_traits.h
    gguf/job_gguf_kv.h
    gguf/job_gguf_context.h
    gguf/job_gguf_reader.h
    gguf/job_gguf_writer.h
    gguf/job_gguf.h
)

# done
set(JOBGGML_OPTIMIZER_HEADERS
    opt/job_ggml_opt_adamw_params.h
    opt/job_ggml_opt_sgd_params.h
    opt/job_ggml_opt_optimizer_params.h
    opt/job_ggml_opt_params.h
    opt/job_ggml_opt_dataset.h
    opt/job_ggml_opt_result.h
    opt/job_ggml_opt_optimizer_schedule.h
    opt/job_ggml_opt_context.h
    opt/job_ggml_opt_step_info.h
    opt/job_ggml_opt_epoch_progress.h
    opt/job_ggml_opt.h
)

set(JOBGGML_QUANTIZATION_HEADERS
    quant/job_ggml_quantization_params.h
    quant/job_ggml_quantization_result.h
    quant/job_ggml_quantizer.h
)

set(JOBGGML_HEADERS
    job_ggml_enums.h
    job_ggml_type_traits.h
    job_ggml_threadpool_params.h
    job_ggml_threadpool.h
    job_ggml_abort_callback.h
    job_ggml.h
)

############ SOURCES
set(JOBGGML_DEVICE_SOURCES
    device/job_ggml_device_caps.cpp
    device/job_ggml_device_props.cpp
    device/job_ggml_device_interface.cpp
    device/job_ggml_device.cpp
    device/job_ggml_device_manager.cpp
)

set(JOBGGML_BACKEND_SOURCES
    backend/job_ggml_backend_reg.cpp
    backend/job_ggml_backend_buffer_type.cpp
    backend/job_ggml_backend_buffer.cpp
    backend/job_ggml_backend_event.cpp
    backend/job_ggml_backend_buffer_view.cpp
    backend/job_ggml_backend.cpp
    backend/job_ggml_backend_sched.cpp
    backend/job_ggml_backend_graph_copy.cpp
    backend/job_ggml_backend_graph_plan.cpp
)

set(JOBGGML_CTX_SOURCES
    ctx/job_ggml_init_params.cpp
    ctx/job_ggml_context_view.cpp
    ctx/job_ggml_context.cpp
    ctx/job_ggml_cgraph.cpp
)

set(JOBGGML_TENSOR_SOURCES
    tensor/job_ggml_tensor_extents.cpp
    tensor/job_ggml_tensor_view.cpp
    tensor/job_ggml_tensor_operation.cpp
    tensor/job_ggml_tensor_data.cpp

    tensor/job_ggml_tensor_fiber.cpp
    tensor/job_ggml_tensor_matrix.cpp
    tensor/job_ggml_tensor_volume.cpp
    tensor/job_ggml_tensor_batch.cpp

    tensor/job_ggml_tensor_layout.cpp
    tensor/job_ggml_tensor.cpp
)

set(JOBGGML_ALLOC_SOURCES
    alloc/job_ggml_tensor_allocator.cpp
    alloc/job_ggml_graph_allocator.cpp
)

set(JOBGGML_GGUF_SOURCES
    gguf/job_gguf_type_traits.cpp
    gguf/job_gguf_kv.cpp
    gguf/job_gguf_tensor_info.cpp
    gguf/job_gguf_init_params.cpp
    gguf/job_gguf_context.cpp
    gguf/job_gguf_reader.cpp
    gguf/job_gguf_writer.cpp
    gguf/job_gguf.cpp
)

set(JOBGGML_OPTIMIZER_SOURCES
    opt/job_ggml_opt_adamw_params.cpp
    opt/job_ggml_opt_sgd_params.cpp
    opt/job_ggml_opt_optimizer_params.cpp
    opt/job_ggml_opt_params.cpp
    opt/job_ggml_opt_dataset.cpp
    opt/job_ggml_opt_result.cpp
    opt/job_ggml_opt_optimizer_schedule.cpp
    opt/job_ggml_opt_context.cpp
    opt/job_ggml_opt_step_info.cpp
    opt/job_ggml_opt_epoch_progress.cpp
    opt/job_ggml_opt.cpp
)


set(JOBGGML_QUANTIZATION_SOURCES
    quant/job_ggml_quantization_params.cpp
    quant/job_ggml_quantization_result.cpp
    quant/job_ggml_quantizer.cpp
)

set(JOBGGML_SOURCES
    job_ggml_enums.cpp
    job_ggml_type_traits.cpp

    job_ggml_threadpool_params.cpp
    job_ggml_threadpool.cpp
    job_ggml_abort_callback.cpp

    job_ggml.cpp
)

set(JOBGGML_COMPILE_THIS
    ${JOBGGML_DEVICE_HEADERS}           ${JOBGGML_DEVICE_SOURCES}
    ${JOBGGML_DEVICE_IMPL_HEADERS}      ${JOBGGML_DEVICE_IMPL_SOURCES}
    ${JOBGGML_BACKEND_HEADERS}          ${JOBGGML_BACKEND_SOURCES}
    ${JOBGGML_CTX_HEADERS}              ${JOBGGML_CTX_SOURCES}
    ${JOBGGML_TENSOR_HEADERS}           ${JOBGGML_TENSOR_SOURCES}
    ${JOBGGML_ALLOC_HEADERS}            ${JOBGGML_ALLOC_SOURCES}
    ${JOBGGML_GGUF_HEADERS}             ${JOBGGML_GGUF_SOURCES}
    ${JOBGGML_OPTIMIZER_HEADERS}        ${JOBGGML_OPTIMIZER_SOURCES}
    ${JOBGGML_QUANTIZATION_HEADERS}     ${JOBGGML_QUANTIZATION_SOURCES}
    ${JOBGGML_HEADERS}                  ${JOBGGML_SOURCES}
)

if(BUILD_SHARED_LIBS)
    add_library(JosephsOddBuilder_Ggml SHARED ${JOBGGML_COMPILE_THIS})
else()
    add_library(JosephsOddBuilder_Ggml STATIC ${JOBGGML_COMPILE_THIS})
    target_compile_definitions(JosephsOddBuilder_Ggml PUBLIC JOBGGML_STATIC_DEFINE)
endif()

set(JOBGGML_EXPORT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/jobggml_export.h")
generate_export_header(JosephsOddBuilder_Ggml
    BASE_NAME JOBGGML
    EXPORT_FILE_NAME "${JOBGGML_EXPORT_HEADER}"
)

set_target_properties(JosephsOddBuilder_Ggml PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobggml"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_compile_definitions(JosephsOddBuilder_Ggml PUBLIC
    $<$<BOOL:${GGML_CPU}>:JOB_GGML_CPU>
    $<$<BOOL:${GGML_CUDA}>:JOB_GGML_CUDA>
    $<$<BOOL:${GGML_VULKAN}>:JOB_GGML_VULKAN>
    $<$<BOOL:${GGML_OPENCL}>:JOB_GGML_OPENCL>
    $<$<BOOL:${GGML_BLAS}>:JOB_GGML_BLAS>
    $<$<BOOL:${GGML_SYCL}>:JOB_GGML_SYCL>
    $<$<BOOL:${GGML_WEBGPU}>:JOB_GGML_WEBGPU>
)


target_include_directories(JosephsOddBuilder_Ggml PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobggml>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/device>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobggml/device>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/device/impl>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobggml/device/impl>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/backend>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobggml/backend>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ctx>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobggml/ctx>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tensor>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobggml/tensor>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/alloc>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobggml/alloc>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gguf>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobggml/gguf>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/opt>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobggml/opt>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/quant>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobggml/quant>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(JosephsOddBuilder_Ggml
    PUBLIC
        Threads::Threads
        $<BUILD_INTERFACE:ggml>
        JosephsOddBuilder_Core
)

if(JOB_LINUX)
    target_link_libraries(JosephsOddBuilder_Ggml PUBLIC atomic)
elseif(JOB_WINDOWS)
    target_compile_options(JosephsOddBuilder_Ggml PRIVATE /wd4251)
endif()

if(GGML_CUDA)
    target_link_libraries(JosephsOddBuilder_Ggml PUBLIC ${JOB_CUDA_LIBS})
endif()

install(TARGETS JosephsOddBuilder_Ggml
    EXPORT JosephsOddBuilderGgmlTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobggml
)

install(FILES ${JOBGGML_EXPORT_HEADER}          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobggml )
install(FILES ${JOBGGML_HEADERS}                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobggml )
# other dirs
install(FILES ${JOBGGML_DEVICE_HEADERS}         DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobggml/device )
install(FILES ${JOBGGML_DEVICE_IMPL_HEADERS}    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobggml/device/impl )

install(FILES ${JOBGGML_BACKEND_HEADERS}        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobggml/backend )
install(FILES ${JOBGGML_CTX_HEADERS}            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobggml/ctx )
install(FILES ${JOBGGML_TENSOR_HEADERS}         DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobggml/tensor )
install(FILES ${JOBGGML_ALLOC_HEADERS}          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobggml/alloc )
install(FILES ${JOBGGML_GGUF_HEADERS}           DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobggml/gguf )
install(FILES ${JOBGGML_OPTIMIZER_HEADERS}      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobggml/opt )
install(FILES ${JOBGGML_QUANTIZATION_HEADERS}   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobggml/quant)

if(JOB_LINUX)
    set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobggml.pc")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/jobggml.pc.in"
        "${PKGCONFIG_FILE}"
        @ONLY
    )
    install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}" )
endif()

install(EXPORT JosephsOddBuilderGgmlTargets
    FILE JosephsOddBuilderGgmlTargets.cmake
    NAMESPACE JOB::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderGgml"
)