cmake_minimum_required(VERSION 3.16)

add_executable(test_job_serializer
    main.cpp
    test_emitter.h
    test_serializer_utils.cpp
    test_job_field.cpp
    test_schema.cpp
    test_emitter.cpp
    test_runtime_object.cpp
    test_iserializer.cpp
    test_writier.cpp
)

set_target_properties(test_job_serializer PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_options(test_job_serializer
    PRIVATE
        -freflection
        -fcontracts
)

target_link_libraries(test_job_serializer PRIVATE
    ${CatchTwo_LIBRARIES}
    ${LibMsgPackC_LIBRARIES}
    ${YAMLCpp_LIBRARIES}
    JosephsOddBuilder_Core
    JosephsOddBuilder_IO
    JosephsOddBuilder_Serializer
)

target_include_directories(test_job_serializer PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
    ${LibMsgPackC_INCLUDE_DIRS}
    ${YAMLCpp_INCLUDE_DIRS}
    ${NlohmannJson_INCLUDE_DIRS}
)

add_test(NAME job_serializer COMMAND test_job_serializer)

