cmake_minimum_required(VERSION 3.16)

option(USE_MMAP_IDS "Use MMAP for the tests" OFF)
set(JOB_USB_TEST_POD_CXX
    test_job_usb_descriptor.cpp
    test_job_usb_location.cpp
    test_job_usb_strings.cpp
    test_job_usb_state.cpp
    test_job_usb_device.cpp
)

set(JOB_USB_TEST_DB_CXX
    test_job_usb_id_line_kernel.cpp
    test_job_usb_id_cursor.cpp
    test_job_usb_id_grammar.cpp
)

if(USE_MMAP_IDS)
    list(APPEND JOB_USB_TEST_DB_CXX test_job_usb_id_database.cpp)
else()
    list(APPEND JOB_USB_TEST_DB_CXX test_job_usb_id_database_file.cpp)
endif()

set(JOB_USB_TEST_CXX
    test_job_usb_udev.cpp
    test_job_usb_parser.cpp
    test_job_usb_monitor.cpp
    test_job_usb.cpp
)

add_executable(test_job_usb
    main.cpp
    ../test_spin_till.h
    ../tests-fast-math-workaround.h
    test_job_usb_fixtures.h
    test_job_usb_utils.h

    ${JOB_USB_TEST_POD_CXX}
    ${JOB_USB_TEST_DB_CXX}
    ${JOB_USB_TEST_CXX}
)


if(USE_MMAP_IDS)
    target_compile_definitions(test_job_usb PRIVATE USE_MMAP_IDS)
endif()


if(NOT JOB_CI_BUILD)
    option(USE_JOSEPHS_LOCAL_IDS "Used for checking against real usb ids" OFF)
    if(USE_JOSEPHS_LOCAL_IDS)
        target_compile_definitions(test_job_usb PRIVATE
            USE_JOSEPHS_LOCAL_IDS=1
        )
    endif()
    target_compile_definitions(test_job_usb PRIVATE
        JOB_USB_TEST_DEVICE_JSON_FILE="${CMAKE_CURRENT_SOURCE_DIR}/fakeDevice.json"
    )

    target_compile_definitions(test_job_usb PRIVATE
        JOB_USB_TEST_DEVICE_YAML_FILE="${CMAKE_CURRENT_SOURCE_DIR}/fakeDevice.yaml"
    )
endif()


target_compile_options(test_job_usb PRIVATE
    -freflection
    -fcontracts
)

target_link_options(test_job_usb PRIVATE
    -fcontracts
)

set_target_properties(test_job_usb PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(test_job_usb PRIVATE
    Threads::Threads
    atomic
    ${CatchTwo_LIBRARIES}
    ${LibUdev_LIBRARIES}
    ${LibUsb-1.0_LIBRARIES}
    JosephsOddBuilder_Threads
    JosephsOddBuilder_IO
    JosephsOddBuilder_Core
    JosephsOddBuilder_Usb
)

target_include_directories(test_job_usb PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_usb COMMAND test_job_usb)
