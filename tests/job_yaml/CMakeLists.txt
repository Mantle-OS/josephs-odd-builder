cmake_minimum_required(VERSION 3.16)

option(JOB_YAML_CPP_BENCH "Need to test against something" ON)


set(JOB_YAML_TEST_FILES
    ## BASE
    test_job_yaml_concepts.cpp
    test_job_yaml_contracts.cpp
    test_job_yaml_cursor.cpp
    test_job_yaml_grammar.cpp
    test_job_yaml_source_range.cpp

    ## LEX
    test_job_yaml_lex.cpp
    test_job_yaml_lex_v1.cpp

    ## Parser utils
    test_job_yaml_indent_stack.cpp
    test_job_yaml_scalar_kernel.cpp
    test_job_yaml_escape_kernel.cpp

    test_job_yaml_key_dispatch.cpp
    test_job_yaml_object_reader.cpp

    test_job_yaml_emitter.cpp

    ## SINK
    test_job_yaml_sink.cpp
    test_job_yaml_node_sink.cpp
    test_job_yaml_object_sink.cpp
    test_job_yaml_validate_sink.cpp
    test_job_yaml_event_sink.cpp

    ## PARSER UTILS
    test_job_yaml_parse_context.cpp
    test_job_yaml_parser_state.cpp
    test_job_yaml_parser_destination.cpp
    test_job_yaml_object_parser_destination.cpp
    test_job_yaml_parser_node_properties.cpp
    test_job_yaml_parser_scalars.cpp
    test_job_yaml_parser_flow.cpp

    # THE PARSER
    test_job_yaml_parser.cpp
    test_job_yaml_parser_v1.cpp
    test_job_yaml_parser_v2.cpp
    test_job_yaml_parser_v3.cpp

    # # NODE
    test_job_yaml_alias_stack.cpp
    test_job_yaml_node_properties.cpp
    test_job_yaml_anchor_table.cpp
    test_job_yaml_node.cpp
    test_job_yaml_node_v1.cpp

    ## SCALAR
    test_job_yaml_scalar_decoder.cpp
    test_job_yaml_decoded_scalar.cpp
    test_job_yaml_block_scalar_decoder.cpp

    # Diagnostics
    test_job_yaml_diagnostic.cpp
    test_job_yaml_diagnostic_generated.cpp
    test_job_yaml_mutations.cpp


    test_job_yaml.cpp
    test_job_yaml_v1.cpp
    test_job_yaml_v2.cpp # NEW to,from string tests
)

if(JOB_YAML_CPP_BENCH AND NOT JOB_CI_BUILD)
    list(APPEND JOB_YAML_TEST_FILES
        test_yaml_cpp_fixtures.h
        test_yaml_cpp_node.cpp
        test_yaml_cpp_parser.cpp
        # # other files
    )
endif()

add_executable(test_job_yaml
    main.cpp
    ../tests-fast-math-workaround.h
    ../transient_test_file.h
    test_job_yaml_utils.h
    test_job_yaml_fixtures.h

    # Generated code from the yaml-schema fork
    test_job_yaml_scalar_fixtures_generated.h
    test_job_yaml_flow_fixtures_generated.h
    test_job_yaml_block_scalar_fixtures_generated.h
    test_job_yaml_diagnostic_fixtures_generated.h
    test_job_yaml_mutation_fixtures_generated.h

    ${JOB_YAML_TEST_FILES}
)

target_compile_options(test_job_yaml PRIVATE
    -freflection
    -fcontracts
    -fcontract-evaluation-semantic=observe
)
target_link_options(test_job_yaml PRIVATE
    -fcontracts
    -fcontract-evaluation-semantic=observe
)

set_target_properties(test_job_yaml PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(test_job_yaml PRIVATE
    ${CatchTwo_LIBRARIES}
    JosephsOddBuilder_Core
)

target_include_directories(test_job_yaml PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

if(JOB_YAML_CPP_BENCH AND NOT JOB_CI_BUILD)
    target_compile_definitions(test_job_yaml PRIVATE
        JOB_YAML_CPP_BENCH
    )

    target_link_libraries(test_job_yaml PRIVATE
        ${YAMLCpp_LIBRARIES}
    )

    target_include_directories(test_job_yaml PRIVATE
        ${YAMLCpp_INCLUDE_DIRS}
    )
endif()

add_test(NAME job_yaml COMMAND test_job_yaml)