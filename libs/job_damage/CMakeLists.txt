project(JosephOddBuild_Damage
    VERSION 0.1
    DESCRIPTION "Damage library for Joseph's Odd Builder"
    LANGUAGES CXX
)

set(JOBDAMAGE_HEADERS
)

set(JOBDAMAGE_SOURCES
)


set(JOBDAMAGE_COMPILE_THIS
    ${JOBDAMAGE_HEADERS}    ${JOBDAMAGE_SOURCES}
)


if(BUILD_SHARED_LIBS)
    add_library(JosephsOddBuilder_Damage SHARED ${JOBDAMAGE_COMPILE_THIS})
else()
    add_library(JosephsOddBuilder_Damage STATIC ${JOBDAMAGE_COMPILE_THIS})
    add_compile_definitions(JOBDAMAGE_STATIC_DEFINE)
endif()

set(JOB_DAMAGE_EXPORT_FILE ${CMAKE_CURRENT_BINARY_DIR}/jobdamage_export.h)
generate_export_header(JosephsOddBuilder_Damage
    BASE_NAME JOBDAMAGE
    EXPORT_FILE_NAME "${JOB_DAMAGE_EXPORT_FILE}"
)

target_link_libraries(JosephsOddBuilder_Damage
    PUBLIC
      JosephsOddBuilder_Core
      JosephsOddBuilder_Threads
      JosephsOddBuilder_IO
    PRIVATE
      Threads::Threads
      atomic
)

set_target_properties(JosephsOddBuilder_Damage PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobdamage"
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_compile_options(JosephsOddBuilder_Damage PRIVATE
    -freflection
    -fcontracts
)

target_link_options(JosephsOddBuilder_Damage PRIVATE
    -fcontracts
)

target_include_directories(JosephsOddBuilder_Damage PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_damage>
    ## exp[ort file]
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

install(TARGETS JosephsOddBuilder_Damage
    EXPORT JosephsOddBuilderDamageTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBDAMAGE_HEADERS}         DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_damage)
install(FILES ${JOB_DAMAGE_EXPORT_FILE}    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_damage)

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/job_damage.pc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobdamage.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}"
    DESTINATION "${PKGCONFIG_INSTALL_DIR}"
)

install(EXPORT JosephsOddBuilderDamageTargets
    FILE JosephsOddBuilderDamageTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderDamage
)
