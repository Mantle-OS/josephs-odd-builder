cmake_minimum_required(VERSION 3.16)

add_executable(test_job_ansi
    main.cpp
    ## utils
    # test_color_utils.cpp
    # test_utf8_endcoder_decoder.cpp
    # test_rgb_color.cpp
    # test_enums.cpp
    # test_job_ansi_utils.cpp
    # test_charset_translator.cpp

    # test_attributes.cpp
    # test_cell.cpp
    # test_grid.cpp
    # test_scrollback.cpp
    # test_screen.cpp
    # test_parser.cpp
    test_dispatch_csi.cpp



)

target_link_libraries(test_job_ansi PRIVATE
    ${CatchTwo_LIBRARIES}
    JosephsOddBuilder_Ansi
    JosephsOddBuilder_Core
    Threads::Threads
    atomic
)

target_include_directories(test_job_ansi PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)
