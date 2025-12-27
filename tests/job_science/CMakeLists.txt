cmake_minimum_required(VERSION 3.16)
add_executable(test_job_science
    main.cpp
    test_verlet_integrator.cpp
    test_barnes_hut.cpp
    test_stencils.cpp
    test_rk4.cpp
    test_euler.cpp
    test_fmm.cpp
)

target_link_libraries(test_job_science PRIVATE
    JosephsOddBuilder_Threads
    ${CatchTwo_LIBRARIES}
    Threads::Threads
    JosephsOddBuilder_Science
)

target_include_directories(test_job_science PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)
