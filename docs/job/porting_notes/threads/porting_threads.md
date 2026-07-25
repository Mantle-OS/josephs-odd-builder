# Porting Threads

The JOB threading framework separates its public C++ interface from the operating-system implementation.

The public declaration files define the portable API and lifecycle state. Each supported platform then provides the native implementation in a corresponding source file.

For example:

```text
job_thread.h
├── linux/job_thread_linux.cpp
├── win32/job_thread_win32.cpp
├── freebsd/job_thread_freebsd.cpp
└── osx/job_thread_osx.cpp
```

The platform implementation must preserve the behavior and lifecycle guarantees defined by the public class. Native APIs may differ, but callers of `JobThread` should observe the same results on every supported operating system.

## 1. `JobThread`

`JobThread` represents one reusable native thread.

The public class owns the platform-independent state:

```cpp
mutable std::mutex                                  m_mutex;
JobThreadOptions                                    m_options;
std::atomic<bool>                                   m_running{false};
RunFunction                                         m_runFunc;
alignas(alignof(std::max_align_t))
unsigned char                                       m_handleStorage[kHandleStorageSize]{};
std::stop_source                                    m_stopSource;

bool                                                m_joinable{false};
std::atomic_flag                                    m_joining{false};
std::atomic<int>                                    m_lastJoinError{0};

std::atomic_flag                                    m_starting{false};
```

The native thread handle is stored inside the opaque `m_handleStorage` buffer. This prevents operating-system types such as `pthread_t` or Windows `HANDLE` from leaking into the public header.

### Required platform implementation

Every `job_thread_PLATFORM.cpp` implementation must define:

```cpp
[[nodiscard]] JobThread::StartResult JobThread::start();
[[nodiscard]] bool JobThread::join() noexcept;
[[nodiscard]] int JobThread::applyScheduling() noexcept;
[[nodiscard]] int JobThread::applyAffinity() noexcept;
```

The implementation also requires a native-handle accessor:

```cpp
static NativeThreadType &provider(unsigned char *storage) noexcept
{
    return *reinterpret_cast<NativeThreadType *>(storage);
}
```

For Linux, the native type is `pthread_t`:

```cpp
static pthread_t &provider(unsigned char *storage) noexcept
{
    return *reinterpret_cast<pthread_t *>(storage);
}
```

The returned reference allows the platform implementation to use the opaque storage directly:

```cpp
pthread_create(
    &provider(m_handleStorage),
    nullptr,
    &JobThread::threadEntry,
    args
);
```

Before adding a new provider, verify that the native handle fits inside the public storage:

```cpp
static_assert(sizeof(NativeThreadType) <= JobThread::kHandleStorageSize);
static_assert(alignof(NativeThreadType) <= alignof(std::max_align_t));
```

If a platform requires a larger or unusually aligned native handle, the public opaque-storage contract must be updated before implementing that provider.

## Lifecycle overview

A normal `JobThread` execution follows this lifecycle:

```text
Idle
  │
  │ start()
  ▼
Starting
  │
  ├── native creation failed ───────────────► Idle
  │
  ├── scheduling or affinity failed ────────► Join/Reap ─► Idle
  │
  └── startup succeeded
          │
          ▼
       Running
          │
          │ requestStop()
          ▼
    Stop Requested
          │
          │ run function returns
          ▼
   Finished but Joinable
          │
          │ join()
          ▼
         Idle
```

A `JobThread` object may be reused after a successful stop-and-join cycle.

## Starting a thread

`start()` begins by acquiring the startup transition flag:

```cpp
if (m_starting.test_and_set(std::memory_order_acq_rel))
    return StartResult::AlreadyRunning;
```

This prevents two callers from attempting to create a native thread at the same time.

After acquiring the flag, the implementation checks whether the object is already running or still owns an unjoined native handle:

```cpp
if (m_running.load(std::memory_order_acquire) || m_joinable) {
    m_starting.clear(std::memory_order_release);
    return StartResult::AlreadyRunning;
}
```

A completed native thread remains joinable until it is reaped. Its handle storage must not be overwritten by another call to `start()`.

### Resetting the stop state

A `std::stop_source` cannot be reset after `request_stop()` has succeeded. Every new execution therefore requires a fresh stop state:

```cpp
m_stopSource = std::stop_source{};
```

Without this reset, the second execution would receive an already-stopped token and could terminate immediately after startup.

The platform implementation then creates a startup promise and packages the required arguments:

```cpp
auto promise = std::make_shared<std::promise<StartResult>>();
auto future  = promise->get_future();

auto *args = new (std::nothrow) JobThreadArgs{
    this,
    promise,
    m_stopSource.get_token()
};
```

The promise synchronizes native startup with the caller. `start()` does not return `Started` until the new thread has completed its scheduling and affinity setup.

### Creating the native thread

On Linux:

```cpp
int const createResult = pthread_create(
    &provider(m_handleStorage),
    nullptr,
    &JobThread::threadEntry,
    args
);
```

If native creation fails:

```cpp
if (createResult != 0) {
    delete args;
    m_joinable = false;
    m_starting.clear(std::memory_order_release);
    return StartResult::ThreadError;
}
```

When creation succeeds, the native handle must eventually be joined:

```cpp
m_joinable = true;
```

The caller then waits for the startup result:

```cpp
StartResult const result = future.get();
```

If the native thread was created but scheduling or affinity setup failed, `start()` immediately joins it before returning:

```cpp
if (result != StartResult::Started && m_joinable) {
    (void)pthread_join(provider(m_handleStorage), nullptr);
    m_joinable = false;
}
```

This returns the object to a clean reusable state after a failed startup.

## Native thread entry

The platform-specific `threadEntry()` owns the argument allocation:

```cpp
std::unique_ptr<JobThreadArgs> args(
    static_cast<JobThreadArgs *>(arg)
);
```

It then performs the native setup required before user code runs.

### Scheduling

The thread first applies its scheduling configuration:

```cpp
if (self->applyScheduling() != 0)
    result = StartResult::SchedulingFailed;
```

### Affinity

Affinity is applied only when requested and only if scheduling has succeeded:

```cpp
if (result == StartResult::Started &&
    self->m_options.pinToCore &&
    self->applyAffinity() != 0) {
    result = StartResult::AffinityFailed;
}
```

These operations run inside the created native thread because scheduling and affinity APIs may apply to the calling thread.

### Publishing startup completion

When native setup succeeds:

```cpp
self->m_running.store(true, std::memory_order_release);
```

The startup promise is then completed:

```cpp
promise->set_value(result);
```

This unblocks the caller waiting inside `start()`.

If startup failed, the thread clears the startup flag and exits:

```cpp
if (result != StartResult::Started) {
    self->m_starting.clear(std::memory_order_release);
    return nullptr;
}
```

## Running user code

After successful startup, the implementation copies the configured run function while holding `m_mutex`:

```cpp
RunFunction functionToRun;

{
    std::lock_guard<std::mutex> lock(self->m_mutex);
    functionToRun = self->m_runFunc;
}
```

The startup transition is now complete:

```cpp
self->m_starting.clear(std::memory_order_release);
```

Further calls to `start()` are rejected by `m_running`.

The thread then invokes either the configured composition callback:

```cpp
if (functionToRun)
    functionToRun(token);
```

or the virtual inheritance path:

```cpp
else
    self->run(token);
```

Both execution styles receive the same `std::stop_token`.

## Requesting a stop

`requestStop()` is platform independent:

```cpp
void requestStop() noexcept
{
    m_stopSource.request_stop();
}
```

This does not forcibly terminate the native thread.

It marks the current stop state as requested. The run function must cooperate by checking the supplied token:

```cpp
while (!token.stop_requested()) {
    // Perform work.
}
```

A run function that never checks its token may prevent `join()` and the destructor from completing.

Calling `requestStop()` multiple times is safe. Once requested, the stop state remains requested for the remainder of that execution.

## Finishing execution

When the run function returns, the native entry function publishes that execution has ended:

```cpp
self->m_running.store(false, std::memory_order_release);
```

At this point:

```text
m_running  == false
m_joinable == true
```

The native thread has finished, but its handle has not yet been reaped.

This is distinct from a fully idle `JobThread`.

A new call to `start()` must remain blocked until `join()` successfully reaps the previous handle.

## Joining

`join()` preserves its original boolean API:

```cpp
[[nodiscard]] bool join() noexcept;
```

The return value means:

```text
true  — the native thread was successfully joined
false — no join completed
```

A false result can mean either:

* the object was not joinable;
* another caller is already joining it;
* the native join operation failed.

The caller may inspect `lastJoinError()` when native error detail is required.

### Preventing simultaneous joins

The implementation first acquires `m_joining`:

```cpp
if (m_joining.test_and_set(std::memory_order_acq_rel))
    return false;
```

Only one caller may attempt to join the native handle.

If the object is not joinable:

```cpp
if (!m_joinable) {
    m_lastJoinError.store(0, std::memory_order_release);
    m_joining.clear(std::memory_order_release);
    return false;
}
```

A zero `lastJoinError()` distinguishes this normal false result from a native join failure.

### Native join failure

On Linux:

```cpp
int const result = pthread_join(
    provider(m_handleStorage),
    nullptr
);
```

If `pthread_join()` fails:

```cpp
if (result != 0) {
    m_lastJoinError.store(result, std::memory_order_release);
    m_joining.clear(std::memory_order_release);
    return false;
}
```

`m_joinable` intentionally remains true.

The implementation has not proven that the native handle was successfully reaped. Clearing `m_joinable` would allow a future `start()` to overwrite potentially valid native-handle storage.

Possible Linux errors include:

```text
EDEADLK — self-join or detected deadlock
EINVAL  — invalid or non-joinable native state
ESRCH   — the stored native thread could not be found
```

A failed join may therefore leave the object blocked from future starts until the lifecycle problem is resolved.

### Successful join

After a successful native join:

```cpp
m_joinable = false;
m_lastJoinError.store(0, std::memory_order_release);
m_joining.clear(std::memory_order_release);
return true;
```

The object has returned to its idle state and may be started again.

## Destruction

The destructor provides RAII cleanup:

```cpp
virtual ~JobThread() noexcept
{
    requestStop();
    (void)join();
}
```

Destruction requests cooperative cancellation and then waits for the native thread to exit.

The run function must not retain references to an object that is being destroyed, and it must honor the stop token so destruction can complete.

A join failure cannot be reported through the destructor because destructors do not return status. Native join errors should therefore be diagnosed before destruction when recovery or reporting matters.

## Platform-provider requirements

Every new `JobThread` provider must preserve these invariants:

1. Only one startup transition may occur at a time.
2. A running thread cannot be started again.
3. An unjoined native handle cannot be overwritten.
4. Every new execution receives a fresh stop state.
5. `start()` returns only after native scheduling and affinity setup completes.
6. A failed native startup is reaped before `start()` returns.
7. Only one caller may join a native handle.
8. Failed joins preserve native-handle ownership.
9. Successful joins return the object to a reusable idle state.
10. `requestStop()` remains cooperative and never forcibly terminates user code.

## Porting checklist

When adding `job_thread_PLATFORM.cpp`, verify:

```text
[ ] Native handle fits inside m_handleStorage.
[ ] provider() returns the correct native handle type.
[ ] Native creation invokes JobThread::threadEntry().
[ ] Startup failures release allocated ThreadArgs.
[ ] Scheduling errors map to SchedulingFailed.
[ ] Affinity errors map to AffinityFailed.
[ ] Native creation errors map to ThreadError.
[ ] Startup promise is completed exactly once.
[ ] Failed startup threads are reaped.
[ ] requestStop() works across repeated executions.
[ ] A fresh stop_source is created before each execution.
[ ] Concurrent start attempts are rejected.
[ ] Concurrent join attempts are rejected.
[ ] Native join errors are stored in m_lastJoinError.
[ ] Failed joins preserve m_joinable.
[ ] Successful joins clear m_joinable.
[ ] The destructor can stop and join an active thread.
[ ] The same JobThread object can be restarted after a successful join.
```

## Regression coverage

The `JobThread` tests cover:

```text
Usage
├── callback-based execution
├── virtual run() execution
├── cooperative stop
├── successful join
└── object reuse

Edge cases
├── destructor cleanup
├── startup-state recovery
├── invalid scheduling options
├── invalid affinity options
├── repeated start/stop churn
├── simultaneous join attempts
├── starting while a join is active
├── joining a non-joinable object
└── native join-error tracking

Optional stress and benchmarks
├── concurrent option/function mutation
├── repeated competing joins
└── startup/stop/join latency
```

These tests serve both as regression protection and as executable examples of the supported `JobThread` lifecycle.
