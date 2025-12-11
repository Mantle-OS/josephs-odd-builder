set(TARGET_NAME job-ansi-example)
project(${TARGET_NAME}
    VERSION 0.1
    DESCRIPTION ""
    LANGUAGES CXX
)

set(ANSIPP_EXAMPLE_DEPS
    Threads::Threads
    atomic
    JosephsOddBuilder_Core
    JosephsOddBuilder_IO
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Ansi
    JosephsOddBuilder_TUI
)
# window.cpp
add_executable(${TARGET_NAME}_window window.cpp )
target_link_libraries(${TARGET_NAME}_window PRIVATE ${ANSIPP_EXAMPLE_DEPS})

# items.cpp
add_executable(${TARGET_NAME}_items items.cpp )
target_link_libraries(${TARGET_NAME}_items PRIVATE ${ANSIPP_EXAMPLE_DEPS})

# focus.cpp
add_executable(${TARGET_NAME}_focus focus.cpp )
target_link_libraries(${TARGET_NAME}_focus PRIVATE ${ANSIPP_EXAMPLE_DEPS})

# test_anchor.cpp
add_executable(${TARGET_NAME}_anchor test_anchors.cpp)
target_link_libraries(${TARGET_NAME}_anchor PRIVATE ${ANSIPP_EXAMPLE_DEPS})

# test_rectangle.cpp
add_executable(${TARGET_NAME}_rectangle test_rectangle.cpp)
target_link_libraries(${TARGET_NAME}_rectangle PRIVATE ${ANSIPP_EXAMPLE_DEPS})

# test_gradient.cpp
add_executable(${TARGET_NAME}_gradient test_gradient.cpp)
target_link_libraries(${TARGET_NAME}_gradient PRIVATE ${ANSIPP_EXAMPLE_DEPS})

# mouse_click_test.cpp
add_executable(${TARGET_NAME}_mouseClick mouse_click_test.cpp)
target_link_libraries(${TARGET_NAME}_mouseClick PRIVATE ${ANSIPP_EXAMPLE_DEPS})

# test_label.cpp
add_executable(${TARGET_NAME}_label test_label.cpp)
target_link_libraries(${TARGET_NAME}_label PRIVATE ${ANSIPP_EXAMPLE_DEPS})


# test_linear_layout.cpp
add_executable(${TARGET_NAME}_linear_layout test_linear_layout.cpp)
target_link_libraries(${TARGET_NAME}_linear_layout PRIVATE ${ANSIPP_EXAMPLE_DEPS})

# test_grid_layout.cpp
add_executable(${TARGET_NAME}_grid_layout test_grid_layout.cpp)
target_link_libraries(${TARGET_NAME}_grid_layout PRIVATE ${ANSIPP_EXAMPLE_DEPS})

# button_test.cpp
add_executable(${TARGET_NAME}_button_test button_test.cpp)
target_link_libraries(${TARGET_NAME}_button_test PRIVATE ${ANSIPP_EXAMPLE_DEPS})

# test_checkbox.cpp
add_executable(${TARGET_NAME}_test_checkbox test_checkbox.cpp)
target_link_libraries(${TARGET_NAME}_test_checkbox PRIVATE ${ANSIPP_EXAMPLE_DEPS})

# test_radio_button.cpp
add_executable(${TARGET_NAME}_test_radio_button test_radio_button.cpp)
target_link_libraries(${TARGET_NAME}_test_radio_button PRIVATE ${ANSIPP_EXAMPLE_DEPS})


