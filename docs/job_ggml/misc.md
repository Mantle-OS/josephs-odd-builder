# Miscellaneous Support

This section documents the supporting JOB GGML types that do not belong to a single higher-level subsystem.

It includes shared enums, type traits, thread-pool configuration and execution support, abort callbacks, and other cross-cutting helpers used throughout the JOB GGML API.

These classes and utilities are intentionally kept separate from devices, backends, tensors, optimization, quantization, and GGUF so those subsystem documents can remain focused on their primary responsibilities.

---

## JOB GGML Enums

`job_ggml_enums.h` provides the strongly typed enum layer shared across JOB GGML.

Most enums map directly to native GGML, GGUF, backend, CPU, or optimizer enums while preserving their native numeric values. Small `constexpr` conversion helpers provide translation between JOB and upstream representations without requiring callers to use raw C enums directly.

The shared enum set includes tensor and quantization types, tensor operations, unary and GLU operations, tensor flags, precision and operation hints, pooling, scaling, sorting, backend device and buffer types, NUMA configuration, scheduler priority, GGUF value types, optimizer loss/build/optimizer types, and related runtime state.

### Explicit Type Support

`JobGgmlType` mirrors the GGML tensor storage types supported by JOB.

Native type validation is intentionally explicit rather than relying only on the numeric range below `GGML_TYPE_COUNT`. This makes newly added upstream GGML types opt-in: a new native type is rejected until JOB has deliberately reviewed and mapped it. :contentReference[oaicite:1]{index=1}

### GGUF Types

`JobGgufType` represents GGUF metadata value types.

The enum is accompanied by compile-time type-size and type-name tables together with helpers for validation, size lookup, name lookup, and conversion to and from native `gguf_type`. Variable-sized GGUF string and array types report a fixed size of zero. :contentReference[oaicite:2]{index=2}

### Optimizer Enums

The optimizer enum set provides JOB representations of GGML loss types, graph build modes, and optimizer implementations.

These map directly to native GGML optimization enums and are guarded by compile-time assertions for selected values so changes in the upstream ABI cannot silently alter JOB's assumptions. :contentReference[oaicite:3]{index=3}

### JobGgmlDeviceImpl

`JobGgmlDeviceImpl` identifies the concrete backend implementation represented by a `JobGgmlDevice`.

Unlike `JobGgmlDeviceType`, which describes the broad native device category such as CPU, GPU, integrated GPU, accelerator, or metadata device, `JobGgmlDeviceImpl` identifies the actual JOB backend implementation.

Known implementations include CPU, BLAS, CUDA, Vulkan, WebGPU, zDNN, VirtGPU, Metal, SYCL, OpenVINO, OpenCL, Hexagon, ZenDNN, CANN, and RPC. Devices without a recognized JOB-specific implementation use `Fallback`.

`deviceImplName()` converts an implementation enum to the backend name used by JOB's discovery indexes, while `deviceImplFromName()` performs the reverse lookup. The implementation-name table is compile-time checked against `JobGgmlDeviceImpl::Count`. :contentReference[oaicite:4]{index=4}

### DeviceManagerState

`DeviceManagerState` represents the lifecycle of `JobGgmlDeviceManager`.

A manager begins in `Uninitialized`, enters `Scanning` while discovering GGML devices and backend registries, transitions to `Ready` after successful discovery, or enters `Error` when discovery cannot complete successfully. :contentReference[oaicite:5]{index=5}

## JobGgmlTypeTraits

`JobGgmlTypeTraits` provides runtime access to the native GGML type-traits information associated with a tensor storage type.

The object wraps a borrowed `ggml_type_traits` entry from GGML's process-wide type-traits table and exposes its information through the JOB enum and C++ interface.

A traits object may be created from either `JobGgmlType`, native `ggml_type`, or an existing native `ggml_type_traits` pointer paired with its corresponding type.

### Type Information

`type()` and `ggmlType()` expose the JOB and native representations of the selected tensor type.

`typeName()` reports the native GGML type name.

`blockSize()` reports the number of logical elements represented by one storage block, while `blockSizeInterleave()` exposes GGML's interleaved block size when applicable.

`typeSize()` reports the native storage size associated with one GGML type block.

`isQuantized()` reports whether the selected storage type is quantized.

### Float Conversion

GGML type traits may expose conversion callbacks for translating between native tensor storage and floating-point values.

`canConvertToFloat()` reports whether a native to-float conversion callback is available.

`canConvertFromFloat()` reports whether the reference from-float conversion callback is available.

`convertToFloat()` converts native tensor storage into `float` values using GGML's type-specific conversion callback.

`convertFromFloatReference()` performs the corresponding reference conversion from floating-point values into the selected GGML storage format.

Both conversion operations validate the traits object, source and destination pointers, element count, and callback availability before invoking GGML.

### Native Traits

`typeTraits()` exposes the borrowed native `ggml_type_traits` structure for direct interoperability with GGML.

The native traits object and its conversion callbacks remain owned by GGML and must not be released by `JobGgmlTypeTraits`.

`setTypeTraits()` replaces the currently selected native type information and refreshes the cached JOB-side fields.

`resetTypeTraits()` restores the object to the native `F32` type traits.


## JobGgmlThreadPoolParams

`JobGgmlThreadPoolParams` represents the configuration used to create and control a GGML CPU thread pool.

The object mirrors the native `ggml_threadpool_params` structure while exposing thread count, scheduling priority, polling behavior, CPU affinity, strict affinity handling, and paused state through a C++ interface.

A parameter object may be created from a requested thread count or from an existing native `ggml_threadpool_params`.

### Thread Count

`nThreads()` reports the number of worker threads requested for the pool.

By default, JOB uses `recommendedThreadCount()`, which derives the value from `std::thread::hardware_concurrency()` and limits it to `GGML_MAX_N_THREADS`.

A valid configuration requires at least one thread and may not exceed `GGML_MAX_N_THREADS`.

### Scheduling and Polling

`prio()` controls the GGML scheduler priority used by the thread pool.

Supported priorities range from low through realtime and are represented by `JobGgmlSchedPriority`.

`poll()` controls the native GGML polling value and must remain within the range accepted by JOB.

### CPU Affinity

The thread-pool CPU mask may be inspected and modified one CPU index at a time through `cpuEnabled()` and `setCpuEnabled()`.

`clearCpuMask()` disables every entry in the affinity mask.

`strictCpu()` controls whether GGML should treat the configured CPU affinity as strict rather than advisory.

### Paused State

`paused()` records whether the thread pool should begin or remain in a paused state.

The value is carried directly into the native `ggml_threadpool_params` structure.

### Native Conversion

`setParams()` replaces the JOB-side configuration from a native `ggml_threadpool_params`.

`params()` rebuilds and returns the native representation from the current JOB-side fields, including the complete CPU mask.

`resetParams()` restores GGML's default thread-pool configuration.

Two `JobGgmlThreadPoolParams` objects may be compared directly. Equality includes the thread count, priority, polling value, strict-affinity state, paused state, and every entry in the CPU mask.



## JobGgmlThreadPool

`JobGgmlThreadPool` owns a native GGML CPU thread pool.

The pool is created from a valid `JobGgmlThreadPoolParams` configuration. During construction, the JOB-side thread count, CPU mask, scheduler priority, polling value, strict-affinity setting, and paused state are converted into the native `ggml_threadpool_params` structure used by GGML.

The native `ggml_threadpool_t` remains owned by `JobGgmlThreadPool` and is released when the wrapper is destroyed.

### Thread Count

`nThreads()` reports the number of worker threads configured when the pool was created.

GGML does not currently expose an implemented public accessor for retrieving the thread count from an existing native thread pool, so JOB retains the configured value alongside the native object.

An invalid or released pool reports a thread count of zero.

### Pause and Resume

`pause()` suspends execution of the native GGML thread pool.

`resume()` allows the thread pool to continue processing work.

Both operations are no-ops when the native pool is not valid.

### Native Access

`threadPool()` exposes the underlying `ggml_threadpool_t` for use with APIs such as the CPU backend thread-pool configuration interface.

The returned native handle remains owned by `JobGgmlThreadPool` and must not be released independently.


## JobGgmlAbortCallback

`JobGgmlAbortCallback` provides a C++ callback bridge for GGML abort handling.

The object stores a `std::function<bool(void *)>` together with an optional borrowed user-data pointer. The callback returns `true` when the associated GGML operation should abort and `false` when execution should continue.

### Callback Invocation

`invoke()` executes the stored C++ callback using the currently configured user-data pointer.

An empty callback is considered invalid and `invoke()` returns false when no callable has been installed.

### User Data

`userData()` exposes the borrowed application-defined pointer supplied to the callback.

`setUserData()` replaces that pointer without transferring ownership. The caller remains responsible for keeping the referenced data valid for as long as the callback may use it.

### Native Callback Bridge

`callback()` exposes the native `ggml_abort_callback` function expected by GGML.

`callbackData()` returns the `JobGgmlAbortCallback` object itself as the callback user-data pointer.

The native callback enters JOB through an internal trampoline, recovers the wrapper object, validates it, and then invokes the stored C++ callable.

The callback object must remain alive for as long as GGML may retain or invoke the native callback and callback-data pair.

### CPU Backend Integration

`JobGgmlAbortCallback` may be installed on a `JobGgmlCpu` through `JobGgmlCpu::setAbortCallback()`.

This allows CPU backend execution to use a stateful C++ abort predicate without callers needing to implement the native GGML callback ABI directly.

