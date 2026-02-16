cmake_minimum_required(VERSION 3.16)
add_executable(test_job_science
    main.cpp

    data/test_zones.cpp
    data/test_disc.cpp
    data/test_forces.cpp
    data/test_composition.cpp
    data/test_particle.cpp

    frames/test_frame_utils.cpp
    frames/test_frame_header.cpp
    frames/test_frame_file_serializer.cpp

    solvers/test_verlet_integrator.cpp
    solvers/test_barnes_hut.cpp
    solvers/test_stencils.cpp
    solvers/test_rk4.cpp
    solvers/test_euler.cpp
    solvers/test_fmm.cpp


    engine/test_engine.cpp

)

target_link_libraries(test_job_science PRIVATE
    JosephsOddBuilder_Threads
    ${CatchTwo_LIBRARIES}
    Threads::Threads
    JosephsOddBuilder_Science
    JosephsOddBuilder_IO
)

target_include_directories(test_job_science PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)
