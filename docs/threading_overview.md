# J.O.B Threading futures (pun absolutely intended)

This file is the “big map” of the threading library:

- What exists today.
- What’s wired but waiting for math.
- What’s still just a twinkle in my scheduler’s eye.

---

## Implementation Status: What We Have vs. What We Need

| # | Algorithm/Pattern | Current Status (What I Have) | What's Missing (Backlog Item) |
|---|-------------------|-------------------------------|-------------------------------|
| 1 | Data Parallelism (`parallel_for`) | **DONE** | Implemented in `job_parallel_for.h`. |
| 2 | Reductions (`parallel_reduce`) | **DONE** | Implemented in `job_parallel_reduce.h`. |
| 3 | Task Graph / Job System | **DONE** | Implemented in `JobThreadGraph` (v1.3). |
| 4 | Pipeline (Stream Parallelism) | **DONE** | `JobPipeline` + `JobPipelineStage` + `JobPipelineSink` built on `JobThreadActor` with fan-out and mailbox-based backpressure. |
| 5 | Work Stealing | **DONE** | `JobWorkStealingScheduler` implemented (mutex-based). |
| 6 | Map-Reduce | **DONE** | Covered by `parallel_reduce`. |
| 7 | Producer-Consumer (Queue) | **DONE** | `JobTaskQueue` and `JobBoundedMPMCQueue` implemented. |
| 8 | Futures & Promises | **DONE** | Native usage in `ThreadPool::submit`. |
| 9 | Asynchronous I/O (`epoll`) | **DONE** | `JobIoAsyncThread` and `JobAsyncEventLoop` implemented. |
| 10 | Barnes-Hut (N-Body) | **DONE** | `BarnesHutTree` and `BarnesHutForceCalculator` implemented. |
| 11 | Fast Multipole Method (FMM) | **HAVE** (Executor): ThreadPool | **NEEDS** (Application): The (very complex) FMM math logic(here be dragons). Library is ready to host it. |
| 12 | Graph Traversal (BFS) | **DONE** | `parallel_bfs(ThreadPool&, const AdjacencyList&, start, visitor)` helper implemented with level-synchronous frontiers, atomic visited flags, and optional depth-aware visitor. Returns `BfsResult { depth, parent }` for introspection. |
| 13 | Graph Traversal (Dijkstra's) | **DONE (sequential core)** | `parallel_dijkstra` over a weighted adjacency list + `DijkstraResult` + `reconstruct_path`. Currently single-threaded internally; API is ready for future parallel PQ/delta-stepping tricks. |
| 14 | Branch and Bound | **DONE** | `parallel_branch_and_bound` helper built on `ThreadPool`, with generic `State`/`Cost`, bound-based pruning, objective/isSolution callbacks, optional timeout, and metrics (expanded/pruned/elapsed). |
| 15 | Monte Carlo Tree Search (MCTS) | **HAVE** (Executor): ThreadPool | **NEEDS** (API): Tree expansion, simulation, backpropagation. |
| 16 | Stencil / Grid | **DONE** | `JobStencilGrid2D` / `JobStencilGrid3D` using double-buffering + `parallel_for`. Synchronization is via the `parallel_for` future (no explicit `std::barrier`). |
| 17 | Cellular Automata | **DONE (via stencil)** | Implemented by `JobStencilGrid2D` + user-defined kernels. See Game of Life “blinker” test as reference. |
| 18 | Euler Integrator | **DONE** |Implemented on top of ThreadPool + force→accel adapters; tested on SHO, damping, convergence, edge cases, and a 10k-particle stress run. |
| 19 | Velocity Verlet | **DONE** | `VerletIntegrator` implemented with KDK/DKD, adapter-based forces. |
| 20 | Runge-Kutta (RK4) | **DONE** | `JobRK4Integrator` implemented on top of ThreadPool + force→accel adapters; validated with harmonic-oscillator and convergence tests. |
| 21 | Reader-Writer Lock | **DONE** | Using `std::shared_mutex` where appropriate. |
| 22 | Fork-Join Model | **DONE** | Via `ThreadPool::submit`, `parallel_for` / `parallel_reduce`, and `JobThreadGraph` (DAG-style joins). |
| 23 | Leapfrog Integration | **DONE** | Covered by `VerletIntegrator`. |
| 24 | Actor Model | **DONE** | `JobThreadActor` (v1.1) implemented, used by pipeline stages and sinks. |
| 25 | Dataflow (Kahn Process Networks) | **PARTIAL** | Topology and backpressure are available via `JobPipeline` + actors; still room for a more formal “dataflow graph” API. |

---

## Where the FHS goes from here in `job_threads`

Current layout of the threading zoo: more incoming

```text
job/libs/job_threads
├── descr
│   ├── job_idescriptor.cpp
│   ├── job_idescriptor.h
│   ├── job_round_robin_descriptor.h
│   ├── job_sporadic_descriptor.h
│   └── job_task_descriptor.h
├── io
│   ├── job_async_event_loop.cpp
│   ├── job_async_event_loop.h
│   ├── job_io_async_thread.cpp
│   └── job_io_async_thread.h
├── job_thread.cpp
├── job_thread.h
├── job_thread_pool.cpp
├── job_thread_pool.h
├── jobthreads.pc.in
├── job_thread_watcher.cpp
├── job_thread_watcher.h
├── queue
│   ├── job_mcmp_queue.h
│   ├── job_task_queue.cpp
│   └── job_task_queue.h
├── sched
│   ├── job_fifo_scheduler.cpp
│   ├── job_fifo_scheduler.h
│   ├── job_isched_policy.h
│   ├── job_round_robin_scheduler.cpp
│   ├── job_round_robin_scheduler.h
│   ├── job_sporadic_scheduler.cpp
│   ├── job_sporadic_scheduler.h
│   ├── job_work_stealing_scheduler.cpp
│   └── job_work_stealing_scheduler.h
└── utils
    ├── job_barnes_hut_calculator.h
    ├── job_barnes_hut_tree.h
    ├── job_branch_and_bound.h
    ├── job_euler_integrator.h
    ├── job_parallel_bfs.h
    ├── job_parallel_dijkstra.h
    ├── job_parallel_for.h
    ├── job_parallel_reduce.h
    ├── job_pipeline.h
    ├── job_pipeline_sink.h
    ├── job_pipeline_stage.h
    ├── job_rk4_integrator.h
    ├── job_stencil_boundary.h
    ├── job_stencil_grid_2D.h
    ├── job_stencil_grid_3D.h
    ├── job_thread_actor.cpp
    ├── job_thread_actor.h
    ├── job_thread_args.h
    ├── job_thread_graph.cpp
    ├── job_thread_graph.h
    ├── job_thread_metrics.h
    ├── job_thread_node.h
    ├── job_thread_options.h
    ├── job_verlet_adapters.h
    ├── job_verlet_concepts.h
    └── job_verlet_integrator.h
```

## The rough mental model:

* job_thread.* + ThreadPool + sched/ + queue/ = core execution engine.
* io/ = event loop / async I/O integration.
* utils/ = “numerical and coordination patterns” built on top of the engine (stencil grids, integrators, actors, pipeline, Barnes–Hut, etc.).

## Table of algos

This table is your shopping list for problem solving: when a new problem shows up, scan this and ask “which hammer fits this nail?”

You don’t have to fill in all the Pros/Cons/Notes right now; they can grow as the library and your experience grow.

| # | Algorithm/Pattern | Category | Time Complexity | Pros (TBD) | Cons (TBD) | Notes (TBD) |
|---|-------------------|----------|-----------------|------------|------------|-------------|
| 1 | **Data Parallelism** (`parallel_for`) | Structured Parallelism | O(N / P) | | | |
| 2 | **Reductions** (`parallel_reduce`) | Structured Parallelism | O(log N) | | | |
| 3 | **Task Graph / Job System** | Dependency-Based | O(critical path) | | | |
| 4 | **Pipeline (Stream Parallelism)** | Producer–Consumer Chain | O(N / min(stage throughput)) | | | |
| 5 | **Work Stealing** | Dynamic Scheduling | O(N / P) amortized | | | |
| 6 | **Map-Reduce (Scatter–Gather)** | Distributed Pattern | O(N / P + log P) | | | |
| 7 | **Producer–Consumer (Queue)** | Async Pattern | O(1) per op | | | |
| 8 | **Futures & Promises** | Async Model | Varies | | | |
| 9 | **Asynchronous I/O** (`epoll` loop) | Event-Driven | ~O(1) per event | | | |
| 10 | **Barnes–Hut (N-Body)** | Hierarchical Approximation | O(N log N) | | | |
| 11 | **Fast Multipole Method (FMM)** | Hierarchical Approximation | O(N) | | | |
| 12 | **Graph Traversal (BFS)** | Search / Ordering | O(V + E) | | | |
| 13 | **Graph Traversal (Dijkstra's)** | Shortest Path | O(E log V) | | | |
| 14 | **Branch and Bound** | Search / Optimization | Exponential (pruned) | | | |
| 15 | **Monte Carlo Tree Search (MCTS)** | Stochastic Search | O(iterations) | | | |
| 16 | **Stencil / Grid (Finite-Difference)** | Spatial Locality | O(N × timesteps) | | | |
| 17 | **Cellular Automata** | Emergent Behavior | O(N × generations) | | | |
| 18 | **Euler Integration** | Numerical ODE Solver | O(N × timesteps) | | | |
| 19 | **Velocity Verlet Integration** | Symplectic ODE Solver | O(N × timesteps) | | | |
| 20 | **Runge–Kutta (RK4)** | Numerical ODE Solver | O(N × timesteps) | | | |
| 21 | **Reader–Writer Lock** | Concurrency Primitive | O(1) acquire | | | |
| 22 | **Fork–Join Model** | Recursive Parallelism | O(N / P) | | | |
| 23 | **Leapfrog Integration** | Symplectic ODE Solver | O(N × timesteps) | | | |
| 24 | **Actor Model** | Message-Passing | Varies | | | |
| 25 | **Dataflow (Kahn Process Networks)** | Stream Processing | O(N) | | | |

As you add RK4, BFS helpers, or a more formal dataflow API, you can start filling in Pros/Cons/Notes with real-world experience (“good for X, terrible for Y, surprised me when Z”).
