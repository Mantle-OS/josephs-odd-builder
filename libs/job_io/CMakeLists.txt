project(JosephOddBuild_IO
    VERSION 0.1
    DESCRIPTION "I/O abstraction layer for Joseph's Odd Builder"
    LANGUAGES CXX
)

set(JOBIO_SOURCES
    pty_io.cpp
    file_io.cpp
    mock_io.cpp
    io_factory.cpp

    job_shared_memory.cpp
    job_file.cpp
    job_mem_lock.cpp
    job_mmap.cpp
    job_range_pool.cpp
    job_page_pool.cpp
    job_size_pool.cpp
    job_arena_pool.cpp

)
set(JOBIO_HEADERS
    io_base.h
    job_permissions.h
    pty_io.h
    file_io.h
    mock_io.h
    job_shared_memory_header.h
    job_shared_memory.h
    io_factory.h

    job_file.h
    job_tmp_file.h

    job_mem_range.h
    job_mem_extent.h
    job_mem_region_concept.h
    job_mem_page.h
    job_mem_span.h
    job_mem_size.h

    job_mem_lock.h
    job_mmap.h

    job_mem_pool.h
    job_range_pool.h
    job_page_pool.h
    job_size_pool.h
    job_arena_pool.h
)

add_library(JosephsOddBuilder_IO SHARED
    ${JOBIO_HEADERS}
    ${JOBIO_SOURCES}
)

# bring in things that need c++23 as a test and also just some randoms
target_compile_options(JosephsOddBuilder_IO PRIVATE
    -freflection
    -fcontracts)

target_link_options(JosephsOddBuilder_IO PRIVATE
    -fcontracts
)

set_target_properties(JosephsOddBuilder_IO PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobio"
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(JosephsOddBuilder_IO PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)

target_link_libraries(JosephsOddBuilder_IO PRIVATE
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
)

install(TARGETS JosephsOddBuilder_IO
    EXPORT JosephsOddBuilderIOTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBIO_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobio )

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobio.pc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobio.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderIOTargets
    FILE JosephsOddBuilderIOTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderIO
)
