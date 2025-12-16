set(TARGET_NAME job-ai-terminal-dashboard-example)
project(job-ai-terminal-dashboard-example
    VERSION 0.1
    DESCRIPTION ""
    LANGUAGES CXX
)

add_executable(job-ai-terminal-dashboard-example main.cpp )

target_link_libraries(job-ai-terminal-dashboard-example PRIVATE
    Threads::Threads
    atomic
    JosephsOddBuilder_Core
    JosephsOddBuilder_Simd
    JosephsOddBuilder_Ansi
    JosephsOddBuilder_IO
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Ansi
    JosephsOddBuilder_TUI
    JosephsOddBuilder_Ai
)

