# J.O.B Threading futures(pun) 

```shell
├── async_event_loop.cpp
├── async_event_loop.h
├── CMakeLists.txt
├── job_burns_and_hut.cpp
├── job_burns_and_hut.h
├── job_io_async_thread.cpp
├── job_io_async_thread.h
├── job_thread.cpp
├── job_thread.h
├── job_thread_metrics.h
├── jobthreads.pc.in
├── job_velocity_verlet.cpp
├── job_velocity_verlet.h
├── task_queue.cpp
├── task_queue.h
├── thread_args.h
├── thread_options.cpp
├── thread_options.h
├── thread_pool.cpp
├── thread_pool.h
├── thread_watcher.cpp
└── thread_watcher.h

1 directory, 22 files

```


## Table of algos

This can be used for any __problem__ later on.

| # | Algorithm/Pattern | Category | Time Complexity | Pros (TBD) | Cons (TBD) | Notes (TBD) |
|---|-------------------|----------|-----------------|------------|------------|-------------|
| 1 | **Data Parallelism** (`parallel_for`) | Structured Parallelism | O(N/P) | | | |
| 2 | **Reductions** (`parallel_reduce`) | Structured Parallelism | O(log N) | | | |
| 3 | **Task Graph / Job System** | Dependency-Based | O(Critical Path) | | | |
| 4 | **Pipeline (Stream Parallelism)** | Producer-Consumer Chain | O(N / min(stage)) | | | |
| 5 | **Work Stealing** | Dynamic Scheduling | O(N/P) amortized | | | |
| 6 | **Map-Reduce (Scatter-Gather)** | Distributed Pattern | O(N/P + log P) | | | |
| 7 | **Producer-Consumer (Queue)** | Async Pattern | O(1) per op | | | |
| 8 | **Futures & Promises** | Async Model | Varies | | | |
| 9 | **Asynchronous I/O** (`io_uring`, `epoll`) | Event-Driven | O(1) per event | | | |
| 10 | **Barnes-Hut (N-Body)** | Hierarchical Approximation | O(N log N) | | | |
| 11 | **Fast Multipole Method (FMM)** | Hierarchical Approximation | O(N) | | | |
| 12 | **Graph Traversal (BFS)** | Search/Ordering | O(V + E) | | | |
| 13 | **Graph Traversal (Dijkstra's)** | Shortest Path | O(E log V) | | | |
| 14 | **Branch and Bound** | Search/Optimization | Exponential (pruned) | | | |
| 15 | **Monte Carlo Tree Search (MCTS)** | Stochastic Search | O(iterations) | | | |
| 16 | **Stencil / Grid (Finite-Difference)** | Spatial Locality | O(N × timesteps) | | | |
| 17 | **Cellular Automata** | Emergent Behavior | O(N × generations) | | | |
| 18 | **Euler Integration** | Numerical ODE Solver | O(N × timesteps) | | | |
| 19 | **Velocity Verlet Integration** | Symplectic ODE Solver | O(N × timesteps) | | | |
| 20 | **Runge-Kutta (RK4)** | Numerical ODE Solver | O(N × timesteps) | | | |
| 21 | **Reader-Writer Lock** | Concurrency Primitive | O(1) acquire | | | |
| 22 | **Fork-Join Model** | Recursive Parallelism | O(N/P) | | | |
| 23 | **Leapfrog Integration** | Symplectic ODE Solver | O(N × timesteps) | | | |
| 24 | **Actor Model** | Message-Passing | Varies | | | |
| 25 | **Dataflow (Kahn Process Networks)** | Stream Processing | O(N) | | | |
            

## Implementation Status: What We Have vs. What We Need

| # | Algorithm/Pattern | Current Status (What We Have) | What's Missing (Backlog Item) |
|---|-------------------|-------------------------------|-------------------------------|
| 1 | Data Parallelism (`parallel_for`) | **HAVE** (Executor): ThreadPool | **NEEDS** (API): A high-level `job_threads::parallel_for(range, function)` that slices the work and `submit()`s it |
| 2 | Reductions (`parallel_reduce`) | **HAVE** (Executor): ThreadPool | **NEEDS** (API): A `job_threads::parallel_reduce` function (more complex, needs tree-like combination) |
| 3 | Task Graph / Job System | **HAVE** (Executor): ThreadPool | **NEEDS** (API): A `JobGraph` class that holds nodes/dependencies and submits "ready" tasks to ThreadPool |
| 4 | Pipeline (Stream Parallelism) | **HAVE** (Primitives): TaskQueue | **NEEDS** (API): A high-level `job_threads::Pipeline` class to formalize chaining multiple queues and thread pools |
| 5 | Work Stealing | **HAVE** (Partial): Shared-queue ThreadPool | **NEEDS** (Refactor): True work-stealing requires v2 ThreadPool where each `JobThread` has its own deque and can "steal" from others |
| 6 | Map-Reduce | **HAVE** (Primitives): ThreadPool | **NEEDS** (API): Combination of `parallel_for` (#1) and `parallel_reduce` (#2) |
| 7 | Producer-Consumer (Queue) | **DONE** | This is exactly what our `TaskQueue` and `ThreadPool` (in its `workerLoop`) implement |
| 8 | Futures & Promises | **DONE** | Already using these in `JobThread::start` and `ThreadPool::submit` |
| 9 | Asynchronous I/O (`epoll`) | **DONE** | This is exactly what our `JobIoAsyncThread` is |
| 10 | Barnes-Hut (N-Body) | **HAVE** (Executor): ThreadPool | **NEEDS** (Application): The `JobBarnesHut` class itself (Octree, force calculations). Lib is ready |
| 11 | Fast Multipole Method (FMM) | **HAVE** (Executor): ThreadPool | **NEEDS** (Application): The (very complex) FMM math logic. Lib is ready |
| 12 | Graph Traversal (BFS) | **HAVE** (Executor): ThreadPool | **NEEDS** (API): A `job_threads::parallel_bfs` helper would be a great addition |
| 13 | Graph Traversal (Dijkstra's) | **HAVE** (Executor): ThreadPool | **NEEDS** (API): Priority-queue based parallel shortest path |
| 14 | Branch and Bound | **HAVE** (Executor): ThreadPool | **NEEDS** (API): Parallel search tree with pruning logic |
| 15 | Monte Carlo Tree Search (MCTS) | **HAVE** (Executor): ThreadPool | **NEEDS** (API): Tree expansion, simulation, backpropagation |
| 16 | Stencil / Grid | **HAVE** (Executor): ThreadPool | **NEEDS** (API): A `job_threads::StencilGrid` class using `std::barrier` (C++20) to sync steps |
| 17 | Cellular Automata | **HAVE** (Executor): ThreadPool | **NEEDS** (API): Grid update rules with double-buffering |
| 18 | Euler Integration | **HAVE** (Executor): ThreadPool | **NEEDS** (Application): Simple ODE stepper implementation |
| 19 | Velocity Verlet | **HAVE** (Executor): ThreadPool | **NEEDS** (Application): The `JobVelocityVerlet` class (already stubbed out!). Lib is ready |
| 20 | Runge-Kutta (RK4) | **HAVE** (Executor): ThreadPool | **NEEDS** (Application): 4-stage ODE integration |
| 21 | Reader-Writer Lock | **HAVE** (C++23): `std::shared_mutex` | **NEEDS** (Decision): Decide where to use `std::shared_mutex` instead of `std::mutex` |
| 22 | Fork-Join Model | **DONE** | This is the model our ThreadPool already uses! `submit()` "forks" work, `future.get()` "joins" it |
| 23 | Leapfrog Integration | **HAVE** (Executor): ThreadPool | **NEEDS** (Application): Symplectic integrator for Hamiltonian systems |
| 24 | Actor Model | **HAVE** (Primitives): `JobThread`, `TaskQueue` | **NEEDS** (API): A `job_threads::Actor` base class that bundles a `JobThread` with a `TaskQueue` (its "mailbox") |
| 25 | Dataflow (Kahn Process Networks) | **HAVE** (Primitives): TaskQueue | **NEEDS** (API): Formalize stream processing with backpressure |



Where the FHS goes from here in jobs_thead 

```shell
job/libs/job_threads$ tree
├── CMakeLists.txt
├── job_thread.cpp
├── job_thread.h
├── job_thread_metrics.h
├── jobthreads.pc.in
├── task_queue.cpp
├── task_queue.h
├── thread_args.h
├── thread_options.cpp
├── thread_options.h
├── thread_pool.cpp
├── thread_pool.h
├── thread_watcher.cpp
└── thread_watcher.h
└── stable/
    ├── async_event_loop.cpp
    ├── async_event_loop.h
    ├── job_io_async_thread.cpp
    ├── job_io_async_thread.h
└── science/
    ├── job_burns_and_hut.cpp
    ├── job_burns_and_hut.h
└── experimental/
    ├── job_velocity_verlet.cpp
    ├── job_velocity_verlet.h

```



