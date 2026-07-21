# Job Threading

Thread creation, scheduling, and a set of `ThreadPool`-backed coordination primitives and parallel algorithms. job_threads links against job_core only.

## Foundational

### JobThreadOptions
Plain options struct controlling how a `JobThread` starts — priority, real-time scheduling policy, core affinity, and heartbeat interval, with `normal()`/`realtimeDefault()` presets.

- `SchedulingPolicy` (`Other`/`FIFO`/`RoundRobin`) is the library's own enum, not a direct alias of any OS scheduler constant, so this header carries zero platform dependency
- `valid()` catches the one combination that can never work — real-time mode paired with the non-real-time `Other` policy — before it ever reaches a backend
- `lockMemory` and `pinToCore` are requests, not guarantees; whether the underlying platform can actually honor them is reported back through `JobThread::StartResult`, not silently assumed

### JobThread
A single OS thread with an optional real-time contract (priority, scheduling policy, core pinning, memory locking).

- `start()` reports one of five outcomes (`Started`/`AlreadyRunning`/`SchedulingFailed`/`AffinityFailed`/`ThreadError`) rather than a bare bool, so a caller that asked for real-time behavior can tell "the thread is running" apart from "the thread is running but not with the guarantees you asked for"
- if the real-time request can't be honored, the thread is reaped internally before `start()` returns — a failed `StartResult` always means there's nothing left to `join()`
- `setRunFunction()` is optional; without one, `run()` is a virtual hook a subclass can override directly instead

### JobSem
A counting semaphore supporting both an in-place/unnamed mode (for intra-process signaling) and a named mode (for cross-process signaling, keyed by a POSIX-style `/name`).

- `open()`/`close()`/`unlink()` govern the named mode's lifetime independently of `init()`/`destroy()`'s in-place mode — the two are mutually exclusive on a single instance
- `wait(int, timeout)` splits one timeout budget across multiple acquisitions rather than reusing the same deadline for each, so N sequential waits can't silently consume N times the requested timeout

## Scheduling

### ISchedPolicy
The scheduling interface `ThreadPool` drives — `enqueue()`/`next()`/`complete()` — with an optional `admit()` hook for policies that need to reject work rather than just order it.

### FifoScheduler
Straight FIFO ordering, no priority lanes.

### JobRoundRobinScheduler
Priority-lane round robin — a queue per priority level, cycling fairly across whichever lanes currently have work rather than draining one priority to empty before moving on.

### JobSporadicScheduler
Earliest-deadline-first scheduling for tasks that carry a deadline and a worst-case execution time (`JobSporadicDescriptor`).

- `admit()` does real admission control: before accepting a new task, it checks total outstanding demand (pending + in-flight + already-reserved) against capacity over the shared deadline horizon, rejecting anything that would make the task set infeasible rather than accepting it and finding out later
- callers can skip `admit()` and go straight to `enqueue()`; the same feasibility check still runs at that point, just without the earlier reservation

### JobWorkStealingScheduler
Per-worker queues with round-robin stealing — an idle worker checks its own queue first, then walks every other worker's queue in turn. Stealing is a plain logical index walk, not core/NUMA-aware.

## Queues and synchronization

### TaskQueue
*Deprecated* — a priority-ordered blocking queue, superseded by the MPMC queue below. Left in place for any one-off use that doesn't need the newer queue's throughput.

### JobBoundedMPMCQueue
A bounded, blocking multi-producer/multi-consumer queue — the actual backing structure behind `JobWorkStealingScheduler`'s per-worker queues.

### JobLatch
A thin wrapper over `std::latch` for one-time countdown synchronization.

## Event loops and async IO

### AsyncEventLoop
A single background thread running posted tasks and timers through one unified wake path — `post()` a task or `addTimer()`/`postDelayed()` a callback, and both land on the same thread in submission order relative to each other.

### JobIoAsyncThread
Extends `AsyncEventLoop` with fd readiness notification — `registerFD()`/`unregisterFD()` alongside the inherited task/timer posting, so I/O callbacks, timers, and posted work all interleave on one thread rather than needing separate synchronization between them.

- `IOEvent` (`Read`/`Write`/`Error`/`HangUp`/`EdgeTriggered`) is the library's own portable event-flag type, not a direct alias of any OS constant
- `globalLoop()` gives every caller in a process access to one shared instance without needing to thread a `JobIoAsyncThread::Ptr` through every constructor that might need to post work

## CTX

### JobFifoCtx / JobRoundRobinCtx / JobSporadicCtx / JobStealerCtx
Each bundles a scheduler, a `ThreadPool` wired to it, and a thread count into one object — the shortest path from "I need a thread pool" to a running one, for whichever scheduling policy fits the workload.

## ThreadPool

### ThreadPool
Fixed-size worker pool driven by an `ISchedPolicy`. `submit()` accepts a plain priority, an explicit descriptor, or neither (defaulting to priority 0), all sharing the same execution path.

- tasks are stored in a sharded hash map (64 shards, keyed by task id) rather than one shared container, to keep contention off a single lock as worker count scales
- `inWorkerThread()` lets code detect whether it's already running on a pool worker, so recursive calls (`parallel_for` calling into a pool from inside a pool task) can fall back to running inline instead of deadlocking waiting for a worker that's already busy being the caller

### ThreadWatcher
Monitors a set of registered threads for exceeding a per-thread timeout, requesting a stop on anything that overruns.

## Utilities

### Structured parallelism
`parallel_for` and `parallel_reduce`/`parallel_reduce_ref` split a range across a `ThreadPool`'s workers, auto-sizing the chunk grain from worker count unless one is given explicitly. `parallel_for` detects when it's already running inside a worker thread and falls back to a plain serial loop rather than trying to recurse into the pool.

### Actor and pipeline coordination
`JobThreadActor<Msg>` is a mailbox-style actor — `post()` a message, `onMessage()` processes it, batched (16 messages per pool submission) rather than one pool task per message. `JobPipelineStage`/`JobPipelineSink`/`JobPipeline` chain actors together into a typed processing pipeline, with fan-out support (one stage feeding several downstream stages).

### Job graphs
`JobThreadGraph` runs a DAG of named tasks against a `ThreadPool` using Kahn's algorithm — add nodes and dependency edges, `run()` executes everything in dependency order, failing fast (and cutting off remaining work) if any node throws.

### Graph traversal and shortest paths
- `parallel_bfs`: level-synchronous frontier traversal, sharded to reduce lock contention when merging each level's frontier back together
- `parallelDijkstra`: delta-stepping-inspired SSSP for non-negative weights, using one atomically-updated (dist, parent) pair per vertex rather than a per-vertex lock; `suggestDelta()` picks a starting delta from a sampled average edge weight when none is given

### Search / optimization
`parallel_branch_and_bound`: parallel exploration of a search tree with bound-based pruning, offloading all but one child of each expanded node to the pool while the calling task continues down the first child itself.

### Monte Carlo
- Monte Carlo engine: a `ThreadPool`-backed sampler for parallel numerical integration (1D and N-D) and generic parallel sample-reduce patterns, with deterministic per-block RNG streams for reproducibility. See also usage in `CorpusChemist` and `MotifToken` [docs](docs/job/ai/token.md).

## Science Utils

If you're looking for:
- FMM (Fast Multipole Method)
- Barnes-Hut
- Verlet
- RK4(Runge Kutta 4)
- Euler
- Stencil grids (2D/3D)

All science-based numerics N-body gravity/long-range interactions, 
stencils & grids, and time integration engines have moved to [job::science](docs/job/science_overview.md).



