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

target_link_libraries(test_job_serializer PRIVATE
    Threads::Threads
    ${CatchTwo_LIBRARIES}
    ${LibMsgPackC_LIBRARIES}
    ${YAMLCpp_LIBRARIES}
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
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

