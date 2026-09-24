cmake_minimum_required(VERSION 3.16)

option(JOB_TEXT_STRESS_UTF8 "These benchmarks are for figuring out the framework with utf8 before moving forward with design" OFF)

option(JOB_TEXT_IO_BRIDGE "Enable JobText and JobIO bridge tests" ON)
set(JOB_TEXT_IO_BRIDGE_TEST_FILES)
if(JOB_TEXT_IO_BRIDGE)
    list(APPEND JOB_TEXT_IO_BRIDGE_TEST_FILES
        test_job_utf_streambuf_io.cpp  ## hello there
    )
endif()

add_executable(test_job_text
    main.cpp
    test_job_text_fixtures.h

    test_job_unicode_scalar.cpp

    test_job_utf8_scalar_kernel.cpp
    test_job_utf8_kernel.cpp
    test_job_ascii_simd_tile.cpp
    test_job_ascii_simd_kernel.cpp
    test_job_utf8_simd_tile.cpp
    test_job_utf8_simd_kernel.cpp
    test_job_utf8.cpp
    # benchmark_utf8_simd.cpp
    # benchmark_ascii_simd.cpp

    test_job_utf16_scalar_kernel.cpp
    test_job_utf16_kernel.cpp
    test_job_utf16_simd_tile.cpp
    test_job_utf16_simd_kernel.cpp
    test_job_utf16.cpp
    # benchmark_utf16_simd.cpp

    test_job_utf32_scalar_kernel.cpp    
    test_job_utf32_kernel.cpp
    test_job_utf32_simd_tile.cpp
    test_job_utf32_simd_kernel.cpp
    test_job_utf32.cpp

    ## Minimal api usage
    test_job_text_transcoder.cpp
    test_job_text_dispatch.cpp
    test_job_text.cpp

    test_job_utf_streambuf.cpp # This first is there anything wrong before we make fancy things

    # benchmark_utf32_simd.cpp

    # test_job_text_benchmarks.cpp

    ${JOB_TEXT_IO_BRIDGE_TEST_FILES}
)

if(JOB_TEXT_STRESS_UTF8)
    target_compile_definitions(test_job_text PRIVATE JOB_TEXT_STRESS_UTF8)
endif()

# target_compile_definitions(test_job_text PRIVATE
#     JOB_TEXT_TEST_ASCII_FILE_1="Whatever we want....."
# )


target_compile_options(test_job_text PRIVATE -freflection -fcontracts)
target_link_options(test_job_text PRIVATE -fcontracts)

set_target_properties(test_job_text PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(test_job_text PRIVATE
    ${CatchTwo_LIBRARIES}
    JosephsOddBuilder_Core
)

if(JOB_TEXT_IO_BRIDGE)
    ## JOBIO Deps
    target_link_libraries(test_job_text PRIVATE
        JosephsOddBuilder_Threads
        JosephsOddBuilder_Crypto
        JosephsOddBuilder_IO
        atomic
        Threads::Threads
    )
endif()

target_include_directories(test_job_text PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_text COMMAND test_job_text)
