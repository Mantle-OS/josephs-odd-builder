cmake_minimum_required(VERSION 3.16)

option(JOB_TEST_NLOHMANN_BENCH "Test against nlohmann::json" ON)

set(JOB_JSON_TEST_FILES
    ## BASE
    test_job_json_grammar.cpp
    test_job_json_concepts.cpp
    test_job_json_source_range.cpp
    test_job_json_cursor.cpp

    # lex
    test_job_json_lex_structure.cpp
    test_job_json_lex_string.cpp
    test_job_json_lex_value.cpp

    # kernels
    test_job_json_number_kernel.cpp
    test_job_json_escape_kernel.cpp
    test_job_json_string_kernel.cpp

    # info
    test_job_json_diagnostic.cpp
    # test_job_json_parser_state.cpp

    ## parser madness
    test_job_json_key_dispatch.cpp
    test_job_json_parser_destination_scalar.cpp
    test_job_json_parser_destination_string.cpp
    test_job_json_parser_destination_wrapper.cpp
    test_job_json_object_parser_destination.cpp

    # The parser
    test_job_json_parser_scalar.cpp
    test_job_json_parser_object.cpp
    test_job_json_parser_array.cpp
    test_job_json_parser_map.cpp
    test_job_json_parser_diagnostic.cpp
    test_job_json_parser_depth.cpp

    # what did you say lol
    test_job_json_emitter_scalar.cpp
    test_job_json_emitter_string.cpp
    test_job_json_emitter_object.cpp
    test_job_json_emitter_array.cpp
    test_job_json_emitter_map.cpp
    test_job_json_emitter_wrapper.cpp
    test_job_json_roundtrip.cpp
)

if(JOB_TEST_NLOHMANN_BENCH AND NOT JOB_CI_BUILD)
    list(APPEND JOB_JSON_TEST_FILES
        test_nlohmann_fixtures.h
        test_benchmark_jobjson_nlohmann.cpp
        test_alloc_jobjson_nlohmann.cpp
        # Other nlohmann benchmark/test files go here.
    )
endif()

add_executable(test_job_json
    main.cpp
    ../tests-fast-math-workaround.h # Fast math is enabled globally, so tests use IEEE byte helpers where needed.
    ../transient_test_file.h    # Helpers for tests that need temporary file I/O.
    test_job_json_utils.h       # One-off utilities and benchmark helpers, including asm barriers.
    test_job_json_fixtures.h    # Shared fixtures for job_json tests.

    # Generated/reviewed material originating from job_json_gen belongs in
    # the library or test fixture files above rather than being generated
    # directly by this CMake target.

    ${JOB_JSON_TEST_FILES}
)

target_compile_options(test_job_json PRIVATE
    -freflection
    -fcontracts
    -fcontract-evaluation-semantic=observe
)

target_link_options(test_job_json PRIVATE
    -fcontracts
    -fcontract-evaluation-semantic=observe
)

set_target_properties(test_job_json PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(test_job_json PRIVATE
    ${CatchTwo_LIBRARIES}
    JosephsOddBuilder_Core
)

target_include_directories(test_job_json PRIVATE ${CatchTwo_INCLUDE_DIRS} )

if(JOB_TEST_NLOHMANN_BENCH AND NOT JOB_CI_BUILD)
    target_compile_definitions(test_job_json PRIVATE
        JOB_TEST_NLOHMANN_BENCH
    )
endif()

add_test( NAME job_json COMMAND test_job_json)

