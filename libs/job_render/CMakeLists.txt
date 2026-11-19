project(JosephOddBuild_Render
    VERSION 0.1
    DESCRIPTION "Render library for Joseph's Odd Builder"
    LANGUAGES CXX
)


set(JOBRENDER_HEADERS
    job_render_application_args.h
    job_render_application.h

    job_damage.h

    input/job_udev_monitor.h
    input/job_input_event.h
    input/job_libinput_context.h
    input/job_input_device.h
)

set(JOBRENDER_SOURCES
    job_render_application_args.cpp
    job_render_application.cpp

    input/job_udev_monitor.cpp
    input/job_libinput_context.cpp
)

set(JOBRENDER_BUFFER_HEADERS
    buffer/job_render_buffer_usage.h
    buffer/job_render_buffer_plane.h
    buffer/job_render_buffer_info.h
    buffer/job_render_buffer.h
)
# set(JOBRENDER_BUFFER_SOURCES)

set(JOBRENDER_IMAGE_HEADERS
    # image/image.h
    # image/image_format.h
)

set(JOBRENDER_IMAGE_SOURCES
    # image/image.h
    # image/image_format.h
)


set(JOBRENDER_TEXT_HEADERS
    # text/font.h
    # text/font_face.h
    # text/font_manager.h

    # text/text_run.h
    # text/text_layout.h
    # text/text_shaper.h

    # text/glyph.h
    # text/glyph_run.h
    # text/glyph_cache.h
    # text/glyph_rasterizer.h
)
set(JOBRENDER_TEXT_SOURCES)

###############################################
# Backends
###############################################

###############################################
# Backends framebuffer
set(JOBRENDER_FB_HEADERS
    fb/job_fb_bitfield.h
    fb/job_fb_type.h
    fb/job_fb_visual.h
    fb/job_fb_capabilities.h

    fb/job_fb_fixed_info.h
    fb/job_fb_variable_info.h

    fb/job_fb_format_result.h
    fb/job_fb_format.h

    fb/job_fb_device.h
    fb/job_fb_buffer.h

    fb/job_fb_presenter.h
)

set(JOBRENDER_FB_SOURCES
    fb/job_fb_fixed_info.cpp
    fb/job_fb_variable_info.cpp
    fb/job_fb_format.cpp
    fb/job_fb_device.cpp
    fb/job_fb_presenter.cpp
)
set(JOBRENDER_FB_BACKEND_HEADERS backend/job_fb_backend.h )
set(JOBRENDER_FB_BACKEND_SOURCES backend/job_fb_backend.cpp )


###############################################
# Backends KMS SIMPLE (No OpenGl) very limited drm_mode_create_dumb
set(JOBRENDER_KMS_SIMPLE_HEADERS
    kms_simple/job_drm_simple_device.h
    kms_simple/job_kms_simple_presenter.h
    kms_simple/job_kms_simple_surface.h

)
set(JOBRENDER_KMS_SIMPLE_SOURCES
    kms_simple/job_drm_simple_device.cpp
    kms_simple/job_kms_simple_presenter.cpp
    kms_simple/job_kms_simple_surface.cpp
)
set(JOBRENDER_KMS_SIMPLE_BACKEND_HEADERS
    backend/job_kms_simple_backend.h
)
set(JOBRENDER_KMS_SIMPLE_BACKEND_SOURCES
    backend/job_kms_simple_backend.cpp
)

###############################################
# Backends KMS -> EGL -> DRM

set(JOBRENDER_DRM_HEADERS
    drm/job_drm_format.h
    drm/job_drm_modifier.h
    drm/job_drm_format_modifier.h
    drm/job_drm_capabilities.h

    drm/job_drm_property.h
    drm/job_drm_property_blob.h
    drm/job_drm_object_properties.h

    drm/job_drm_resources.h
    drm/job_drm_plane_resources.h
    drm/job_drm_encoder.h

    drm/job_drm_device.h
    drm/job_drm_mode.h
    drm/job_drm_connector.h
    drm/job_drm_crtc.h
    drm/job_drm_plane.h
    drm/job_drm_framebuffer_registration.h
    drm/job_drm_framebuffer.h

    drm/job_drm_damage_rect.h
    drm/job_drm_damage.h
    drm/job_drm_atomic_state.h
    drm/job_drm_atomic_request.h
)

set(JOBRENDER_DRM_SOURCES
    drm/job_drm_capabilities.cpp
    drm/job_drm_resources.cpp
    drm/job_drm_framebuffer_registration.cpp
    drm/job_drm_device.cpp
    drm/job_drm_atomic_request.cpp
)

set(JOBRENDER_GBM_HEADERS
    gbm/job_gbm_format.h
    gbm/job_gbm_modifier.h
    gbm/job_gbm_buffer_plane.h
    gbm/job_gbm_device.h
    gbm/job_gbm_buffer.h
    gbm/job_gbm_surface.h
    gbm/job_gbm_buffer_lease.h
)

set(JOBRENDER_GBM_SOURCES
    gbm/job_gbm_device.cpp
    gbm/job_gbm_buffer.cpp
    gbm/job_gbm_surface.cpp
    gbm/job_gbm_buffer_lease.cpp
)

set(JOBRENDER_EGL_HEADERS
    egl/job_egl_extensions.h
    egl/job_egl_display.h
    egl/job_egl_config.h
    egl/job_egl_context.h
    egl/job_egl_surface.h
)
set(JOBRENDER_EGL_SOURCES
    egl/job_egl_extensions.cpp
    egl/job_egl_display.cpp
    egl/job_egl_config.cpp
    egl/job_egl_context.cpp
    egl/job_egl_surface.cpp
)

set(JOBRENDER_PRESENT_HEADERS
    present/job_present_frame.h         # kms
    present/job_present_entry.h         # kms
    present/job_present_queue.h         # stl/posix
    present/job_kms_presenter_restore.h # kms
    present/job_kms_presenter.h         # kms
)

set(JOBRENDER_PRESENT_SOURCES
    present/job_present_queue.cpp
    present/job_kms_presenter.cpp
)

set(JOBRENDER_SYNC_HEADERS
    sync/job_render_fence.h    # stl posix
    sync/job_sync_file.h       # stl, linux linux/sync_file.h
    sync/job_drm_syncobj.h     # DRM
    sync/job_dma_buf_sync.h    # linux/dma-buf.h, posix stl
)

set(JOBRENDER_SYNC_SOURCES
    sync/job_render_fence.cpp   # stl posix
    sync/job_sync_file.cpp      # stl, linux linux/sync_file.h

    sync/job_drm_syncobj.cpp    # drm

    sync/job_dma_buf_sync.cpp   # linux/dma-buf.h, posix stl
)

set(JOBRENDER_KMS_BACKEND_HEADERS
    backend/job_kms_format_negotiator.h
    backend/job_kms_backend.h
)
set(JOBRENDER_KMS_BACKEND_SOURCES
    backend/job_kms_format_negotiator.cpp
    backend/job_kms_backend.cpp
)

###############################################
# backend wayland

# scanner cursor and shared friends
set(JOBRENDER_WAYLAND_HEADERS)
set(JOBRENDER_WAYLAND_SOURCES)

# server
set(JOBRENDER_WAYLAND_SERVER_HEADERS

)
set(JOBRENDER_WAYLAND_SERVER_SOURCES

)

# client
set(JOBRENDER_WAYLAND_CLIENT_HEADERS

)
set(JOBRENDER_WAYLAND_CLIENT_SOURCES

)



set(JOBRENDER_COMPILE_THIS
    ${JOBRENDER_HEADERS}        ${JOBRENDER_SOURCES}
    # ${JOBRENDER_TEXT_HEADERS}   ${JOBRENDER_TEXT_SOURCES}
    # ${JOBRENDER_IMAGE_HEADERS}   ${JOBRENDER_IMAGE_SOURCES}
)

if(JOB_BUILD_RENDER_FB)
    list(APPEND JOBRENDER_COMPILE_THIS
        ${JOBRENDER_FB_HEADERS}             ${JOBRENDER_FB_SOURCES}
        ${JOBRENDER_FB_BACKEND_HEADERS}     ${JOBRENDER_FB_BACKEND_SOURCES}
    )
endif()

if(JOB_BUILD_RENDER_KMS)

    if(HAS_SIMPLE_KMS)
        list(APPEND JOBRENDER_COMPILE_THIS
            ${JOBRENDER_KMS_SIMPLE_HEADERS}         ${JOBRENDER_KMS_SIMPLE_SOURCES}
            ${JOBRENDER_KMS_SIMPLE_BACKEND_HEADERS} ${JOBRENDER_KMS_SIMPLE_BACKEND_SOURCES}
        )
    endif()

    if(HAS_FULL_KMS)
        list(APPEND JOBRENDER_COMPILE_THIS
            ${JOBRENDER_BUFFER_HEADERS}         ${JOBRENDER_BUFFER_SOURCES}
            ${JOBRENDER_DRM_HEADERS}            ${JOBRENDER_DRM_SOURCES}
            ${JOBRENDER_GBM_HEADERS}            ${JOBRENDER_GBM_SOURCES}
            ${JOBRENDER_EGL_HEADERS}            ${JOBRENDER_EGL_SOURCES}
            ${JOBRENDER_SYNC_HEADERS}           ${JOBRENDER_SYNC_SOURCES}
            ${JOBRENDER_PRESENT_HEADERS}        ${JOBRENDER_PRESENT_SOURCES}
            ${JOBRENDER_KMS_BACKEND_HEADERS}    ${JOBRENDER_KMS_BACKEND_SOURCES}
        )
    endif()

endif()

if(JOB_BUILD_RENDER_WAYLAND)
    list(APPEND JOBRENDER_COMPILE_THIS ${JOBRENDER_WAYLAND_HEADERS} ${JOBRENDER_WAYLAND_SOURCES})
    if(JOB_BUILD_RENDER_WAYLAND_SERVER)
        list(APPEND JOBRENDER_COMPILE_THIS ${JOBRENDER_WAYLAND_SERVER_HEADERS} ${JOBRENDER_WAYLAND_SERVER_SOURCES})
    elseif(JOB_BUILD_RENDER_WAYLAND_CLIENT)
        list(APPEND JOBRENDER_COMPILE_THIS ${JOBRENDER_WAYLAND_CLIENT_HEADERS} ${JOBRENDER_WAYLAND_CLIENT_SOURCES})
    endif()
endif()


if(BUILD_SHARED_LIBS)
    add_library(JosephsOddBuilder_Render SHARED ${JOBRENDER_COMPILE_THIS})
else()
    add_library(JosephsOddBuilder_Render STATIC ${JOBRENDER_COMPILE_THIS})
    target_compile_definitions(JosephsOddBuilder_Render PUBLIC JOBRENDER_STATIC_DEFINE)
endif()

set(JOB_RENDER_EXPORT_FILE ${CMAKE_CURRENT_BINARY_DIR}/jobrender_export.h)
generate_export_header(JosephsOddBuilder_Render
    BASE_NAME JOBRENDER
    EXPORT_FILE_NAME "${JOB_RENDER_EXPORT_FILE}"
)

target_link_libraries(JosephsOddBuilder_Render
    PUBLIC
        JosephsOddBuilder_Core
        JosephsOddBuilder_Threads
        JosephsOddBuilder_IO
        # JosephsOddBuilder_Damage
    PRIVATE
        Threads::Threads
        atomic
        ${XkbCommon_LIBRARIES}
        ${LibInput_LIBRARIES}
        ${LibUdev_LIBRARIES}
        ${Harfbuzz_LIBRARIES}
)

if(JOB_BUILD_RENDER_KMS)
    if(HAS_SIMPLE_KMS)
        target_link_libraries(JosephsOddBuilder_Render PRIVATE
            ${LibDrm_LIBRARIES}
        )
    endif()

    if(HAS_FULL_KMS)
        target_link_libraries(JosephsOddBuilder_Render PRIVATE
            ${LibDrm_LIBRARIES}
            ${LibDri_LIBRARIES}
            ${LibGbm_LIBRARIES}
            ${LibEgl_LIBRARIES}
        )
    endif()
endif()

if(JOB_BUILD_RENDER_WAYLAND)
    target_link_libraries(JosephsOddBuilder_Render PRIVATE
        ${WaylandCursor_LIBRARIES}
        ${LibEgl_LIBRARIES}
        ${WaylandEgl_LIBRARIES}
    )
    if(JOB_BUILD_RENDER_WAYLAND_CLIENT)
        target_link_libraries(JosephsOddBuilder_Render PRIVATE
            ${XWayland_LIBRARIES}
            ${WaylandClient_LIBRARIES}
        )
    endif()

    if(JOB_BUILD_RENDER_WAYLAND_SERVER)
        target_link_libraries(JosephsOddBuilder_Render PRIVATE
            ${WaylandServer_LIBRARIES}
            ${XWayland_LIBRARIES}
        )
    endif()
endif()


set_target_properties(JosephsOddBuilder_Render PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobrender"
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_compile_options(JosephsOddBuilder_Render PRIVATE
    -freflection
    -fcontracts
)

target_link_options(JosephsOddBuilder_Render PRIVATE
    -fcontracts
)

target_include_directories(JosephsOddBuilder_Render PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_render>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/backend>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_render/backend>

    ## temp
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/input>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_render/input>

    # export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

if(JOB_BUILD_RENDER_KMS)
    if(HAS_SIMPLE_KMS)
        target_include_directories(JosephsOddBuilder_Render PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/kms_simple>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_render/kms_simple>
        )
        target_include_directories(JosephsOddBuilder_Render PRIVATE
            ${LibDrm_INCLUDE_DIRS}
        )
    endif()

    if(HAS_FULL_KMS)
        target_include_directories(JosephsOddBuilder_Render PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/buffer>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_render/buffer>

            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/drm>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_render/drm>

            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gbm>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_render/gbm>

            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/egl>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_render/egl>

            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sync>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_render/sync>

            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/present>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_render/present>
        )
        target_include_directories(JosephsOddBuilder_Render PRIVATE
            # PkgConfig::LibDrm
            ${LibGbm_INCLUDE_DIRS}
            ${LibEgl_INCLUDE_DIRS}
        )
    endif()
endif()


if(JOB_BUILD_RENDER_FB)
    target_include_directories(JosephsOddBuilder_Render PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/fb>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_render/fb>
    )
endif()



if(JOB_BUILD_RENDER_WAYLAND)
    target_include_directories(JosephsOddBuilder_Render PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/wayland>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_render/wayland>
    )
    target_compile_definitions(JosephsOddBuilder_Render PUBLIC JOB_RENDER_HAS_WAYLAND)

    if(JOB_BUILD_RENDER_WAYLAND_CLIENT)
        target_include_directories(JosephsOddBuilder_Render PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/wayland-client>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_render/wayland_client>
        )
        target_compile_definitions(JosephsOddBuilder_Render PUBLIC JOB_RENDER_HAS_WAYLAND_CLIENT)
    endif()

    if(JOB_BUILD_RENDER_WAYLAND_SERVER)
        target_include_directories(JosephsOddBuilder_Render PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/wayland-server>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_render/wayland_server>
        )
        target_compile_definitions(JosephsOddBuilder_Render PUBLIC JOB_RENDER_HAS_WAYLAND_SERVER)
    endif()
endif()


install(TARGETS JosephsOddBuilder_Render
    EXPORT JosephsOddBuilderRenderTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBRENDER_HEADERS}              DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_render)
install(FILES ${JOB_RENDER_EXPORT_FILE}         DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_render)

if(JOB_BUILD_RENDER_KMS)

    if(HAS_SIMPLE_KMS)
        install(FILES ${JOBRENDER_KMS_SIMPLE_HEADERS}           DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_render/kms_simple)
        install(FILES ${JOBRENDER_KMS_SIMPLE_BACKEND_HEADERS}   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_render/backend)
    endif()

    if(HAS_FULL_KMS)
        install(FILES ${JOBRENDER_BUFFER_HEADERS}       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_render/buffer)
        install(FILES ${JOBRENDER_DRM_HEADERS}          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_render/drm)
        install(FILES ${JOBRENDER_GBM_HEADERS}          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_render/gbm)
        install(FILES ${JOBRENDER_EGL_HEADERS}          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_render/egl)
        install(FILES ${JOBRENDER_SYNC_HEADERS}         DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_render/sync)
        install(FILES ${JOBRENDER_PRESENT_HEADERS}      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_render/present)
        install(FILES ${JOBRENDER_KMS_BACKEND_HEADERS}  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_render/backend)
    endif()

endif()

if(JOB_BUILD_RENDER_FB)
    install(FILES ${JOBRENDER_FB_HEADERS}           DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_render/fb)
    install(FILES ${JOBRENDER_FB_BACKEND_HEADERS}   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_render/backend)
endif()

if(JOB_BUILD_RENDER_WAYLAND)
    install(FILES ${JOBRENDER_WAYLAND_HEADERS}  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_render/wayland)
    if(JOB_BUILD_RENDER_WAYLAND_CLIENT)
        install(FILES ${JOBRENDER_WAYLAND_CLIENT_HEADERS}   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_render/wayland_client)
    endif()

    if(JOB_BUILD_RENDER_WAYLAND_SERVER)
        install(FILES ${JOBRENDER_WAYLAND_SERVER_HEADERS}   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_render/wayland_server)
    endif()
endif()



# TODO later once all deps are really known
# set(PKGCONFIG_INSTALL_DIR   "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
# set(PKGCONFIG_FILE          "${CMAKE_CURRENT_BINARY_DIR}/job_render.pc")
# configure_file(
#     "${CMAKE_CURRENT_SOURCE_DIR}/jobrender.pc.in"
#     "${PKGCONFIG_FILE}"
#     @ONLY
# )
# install(FILES "${PKGCONFIG_FILE}"   DESTINATION "${PKGCONFIG_INSTALL_DIR}")

# if(JOB_BUILD_RENDER_KMS)
#     set(PKGCONFIG_KMS_FILE "${CMAKE_CURRENT_BINARY_DIR}/job_render_kms.pc")
#     configure_file(
#         "${CMAKE_CURRENT_SOURCE_DIR}/jobrender_kms.pc.in"
#         "${PKGCONFIG_KMS_FILE}"
#         @ONLY
#     )
#     install(FILES "${PKGCONFIG_KMS_FILE}"   DESTINATION "${PKGCONFIG_INSTALL_DIR}")
# endif()

# if(JOB_BUILD_RENDER_FB)
#     set(PKGCONFIG_FB_FILE "${CMAKE_CURRENT_BINARY_DIR}/job_render_fb.pc")
#     configure_file(
#         "${CMAKE_CURRENT_SOURCE_DIR}/jobrender_fb.pc.in"
#         "${PKGCONFIG_FB_FILE}"
#         @ONLY
#     )
#     install(FILES "${PKGCONFIG_FB_FILE}"    DESTINATION "${PKGCONFIG_INSTALL_DIR}")
# endif()

install(EXPORT JosephsOddBuilderRenderTargets
    FILE JosephsOddBuilderRenderTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderRender
)