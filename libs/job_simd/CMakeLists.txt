project(JosephOddBuild_Simd
    VERSION 0.1
    DESCRIPTION "Simd Lib for Joseph's Odd Builder"
    LANGUAGES CXX
)

set(JOBSIMD_SOURCES
    simd_provider.cpp
)
set(JOBSIMD_HEADERS
    rounding_mode.h
    transpose.h
    simd_provider.h
    simd_math.h
)

# these LOAD when the providor is used
if(JOB_AVX_512_VNNI_FLAG OR JOB_AVX_512_FLAG)
    list(APPEND  JOBSIMD_HEADERS simd_avx512.h)
elseif(JOB_AVX_FLAG OR JOB_AVX_TWO_FLAG OR JOB_AVX_VNNI_FLAG)
    list(APPEND  JOBSIMD_HEADERS simd_avx.h)
elseif(JOB_AVX_NEON_FLAG)
    list(APPEND  JOBSIMD_HEADERS simd_arm.h)
else()
    message(FATAL_ERROR "AVX mismatch you have to set it up. see cmake/build_options")
endif()

if(BUILD_SHARED_LIBS)
add_library(JosephsOddBuilder_Simd SHARED
    ${JOBSIMD_HEADERS}
    ${JOBSIMD_SOURCES}
)
else()
    add_library(JosephsOddBuilder_Simd STATIC
        ${JOBSIMD_HEADERS}
        ${JOBSIMD_SOURCES}
    )
    add_compile_definitions(JOBSIMD_STATIC_DEFINE)
endif()

## nothing really needs this .... Yet but I am here right now so.
set(JOB_SIMD_EXPORT_FILE ${CMAKE_CURRENT_BINARY_DIR}/jobsimd_export.h)
generate_export_header(JosephsOddBuilder_Simd
    BASE_NAME JOBSIMD
    EXPORT_FILE_NAME "${JOB_SIMD_EXPORT_FILE}"
)

if(JOB_LINUX)
    target_link_libraries(JosephsOddBuilder_Simd PUBLIC
        Threads::Threads
        atomic
    )
elseif(JOB_WINDOWS)
    target_link_libraries(JosephsOddBuilder_Simd PUBLIC
        ws2_32
    )
endif()

set_target_properties(JosephsOddBuilder_Simd PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobsimd"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(JosephsOddBuilder_Simd PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobsimd/>
    ## export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

install(TARGETS JosephsOddBuilder_Simd
    EXPORT JosephsOddBuilderSimdTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBSIMD_HEADERS}        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobsimd )
install(FILES ${JOB_SIMD_EXPORT_FILE}   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobsimd )


if(JOB_LINUX)
    set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobsimd.pc")

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/jobsimd.pc.in"
        "${PKGCONFIG_FILE}"
        @ONLY
    )

    install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
endif()

install(EXPORT JosephsOddBuilderSimdTargets
    FILE JosephsOddBuilderSimdTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderSimd
)
