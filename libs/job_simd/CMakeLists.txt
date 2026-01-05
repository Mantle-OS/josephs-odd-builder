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

if(HAS_AVX)
    list(APPEND  JOBSIMD_HEADERS simd_avx.h)
elseif(HAS_NEON)
    list(APPEND  JOBSIMD_HEADERS simd_arm.h)
endif()


add_library(JosephsOddBuilder_Simd SHARED ${JOBSIMD_HEADERS} ${JOBSIMD_SOURCES})

set_target_properties(JosephsOddBuilder_Simd PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobsimd"
)

target_include_directories(JosephsOddBuilder_Simd PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobsimd/>
)

install(TARGETS JosephsOddBuilder_Simd
    EXPORT JosephsOddBuilderSimdTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBSIMD_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobsimd
)

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobsimd.pc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobsimd.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderSimdTargets
    FILE JosephsOddBuilderSimdTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderSimd
)

# CHECKPOINT: v1.0
