project(job-ai-terminal-dashboard-example
    VERSION 0.1
    DESCRIPTION ""
    LANGUAGES CXX
)

add_executable(job-ai-terminal-dashboard-example
    main.cpp
)

set_target_properties(job-ai-terminal-dashboard-example PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)

target_compile_options(job-ai-terminal-dashboard-example PRIVATE
    -freflection
    -fcontracts
)

target_link_options(job-ai-terminal-dashboard-example PRIVATE
    -fcontracts
)

target_link_libraries(job-ai-terminal-dashboard-example PRIVATE
    Threads::Threads
    atomic
    JosephsOddBuilder_Core
    JosephsOddBuilder_Ansi
    JosephsOddBuilder_IO
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Ansi
    JosephsOddBuilder_TUI
    JosephsOddBuilder_Ai
)