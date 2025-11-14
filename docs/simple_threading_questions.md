# Threading & Parallelism Algorithms for Package Building
## Context: Building a Linux Distribution from Scratch
### Problem Domain: Fetch → Patch → Configure → Build → Install → Package

---

## Algorithm Catalog (boiler plate that we can use for each "problem")

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

---

## Problem Decomposition Matrix

### Sub-Problems to Map Algorithms Against:

1. **Fetch** (Download N source tarballs)
   - Parallel network I/O
   - Variable latency, bandwidth
   - Priorities (toolchain first, then packages)
   
2. **Patch** (Apply diffs to source trees)
   - File I/O heavy
   - Independent per-package
   - Potential for textual conflicts
   
3. **Configure** (Run `./configure` scripts)
   - CPU-light, I/O-heavy
   - Generates Makefiles
   - Some dependencies (need headers from libc before configuring gcc)
   
4. **Build** (Compile source → binaries)
   - CPU-bound
   - Deep dependency graphs (gcc needs binutils, glibc needs kernel headers)
   - Variable compilation times (gcc: 2 hours, busybox: 5 minutes)
   
5. **Install** (Copy binaries to staging area)
   - I/O-bound
   - Fast, sequential
   - File conflicts possible
   
6. **Package** (Create compressed archives with manifests)
   - CPU-bound (compression)
   - I/O-bound (reading files)
   - Independent per-package

---

## Creative Mapping Ideas (To Explore)

- **Barnes-Hut for Fetch**: Network packets as "dust particles" in latency/size space
- **Verlet for Fetch**: Downloads as particles with velocity (bandwidth) and acceleration (congestion)
- **Stencil for Thread Pool**: Worker threads as grid cells, work "diffuses" via local stealing
- **MCTS for Build Order**: Explore different dependency resolution orders, backtrack on failures
- **Cellular Automata for Pipeline**: Each stage's state depends on neighbors (fetch→patch→configure)
- **Actor Model for Task Graph**: Each package is an actor, messages are "I need X to proceed"

---

## Next Steps

1. Pick a sub-problem (e.g., **Fetch**)
2. Select 3-5 algorithms from the table
3. For each, fill in:
   - **Pros**: Why it *might* work (the creative angle)
   - **Cons**: Why it *might* crash & burn (overhead, impedance mismatch)
   - **Notes**: Implementation sketch, expected behavior
4. Prototype the most promising one
5. Benchmark against naive approach (e.g., `asyncio` with priority queue)

---

*"In theory, there is no difference between theory and practice. In practice, there is."* — Yogi Berra (or someone like him)
