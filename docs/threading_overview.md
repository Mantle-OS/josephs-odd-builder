# Job Threading

- **Core** = work + workers + stop/shutdown
- **Scheduling** = ordering rules for work
- **Queues + sync** = coordination primitives
- **IO/event loop** = timers and fd readiness without extra thread sprawl
- **Utils** = parallel patterns built on the engine
- **Ctx** =   scheduler + thread pool + thread count

## Threading details

### Scheduling
Scheduling is policy-driven. Multiple schedulers exist to cover different kinds of workloads:
- **FIFO**: simple ordering
- **Round-robin**: fairness across lanes/priority classes
- **Work stealing**: load balancing under uneven task duration
- **Sporadic / deadline-aware**: a timing-intent lane

Conceptually: the pool requests the next runnable task; the scheduler decides what that is.

## Queues and synchronization

Common coordination primitives:
- semaphore-based wake/sleep for workers
- bounded MPMC queue for mailbox/backpressure patterns
- task queues where ordering matters

### Event loops and async IO

- an event loop that runs posted tasks and timers
- an epoll-backed IO loop that handles fd readiness through the same pathway

This concentrates:
- periodic work (timers)
- readiness-triggered work (fd callbacks)
- queued work (posted tasks)

### CTX
Each context bundles:
- a scheduler instance
- a thread pool instance wired to that scheduler
- a stored thread-count used for construction

### Utilities

patterns built on top of a pool + scheduler.

### Structured parallelism
- data-parallel loops for splitting work across workers
- reductions for parallel aggregation

### Actor + pipeline coordination
- actor-style mailbox processing using bounded queues for backpressure
- pipeline stage chaining (and fan-out) built from actors and mailboxes

### Job graphs / dependency execution
- task graph execution: nodes + dependencies executed in valid order

### Graph traversal and shortest paths
- BFS (level-synchronous): frontier-based traversal with visited tracking; returns traversal structure (depth/parent info)
- SSSP (delta-stepping style): non-negative weighted shortest paths with bucketed relaxation; returns distance/parent info and supports path recovery

### Search / optimization
- branch and bound: parallel exploration with pruning using bounds and an objective

### Monte Carlo
- Monte Carlo engine: a `ThreadPool`-backed sampler that estimates integrals by drawing random points and reducing results in parallel. Includes 1D and N-D estimators (sample `f(x)` over bounds, average, then scale by the domain volume). Randomness uses `SplitMix64` with deterministic per-sample seeding.
- Generic sampling reducer: a `sampleReduce(...)` helper that runs “blocks of samples” in parallel. Each block gets its own deterministic RNG stream (seed mixed with block index), accumulates into a caller-defined accumulator, and merges accumulators via a caller-provided reduce function. Chunk size is auto-chosen unless overridden.
- Example usage: `job::ai::token::CorpusChemist` uses `job::threads::MonteCarloEngine::sampleReduce` to randomly probe a text corpus, hash fixed-length fragments, and keep a small Top-K of the most frequently observed candidates. It returns the best-scoring fragment as a “reactive molecule” substring for the requested length.
