# Warning

JOB GGML is currently under active development. APIs, class boundaries, and internal implementations may change as the library evolves.

---

# JOB GGML

JOB GGML is a modern C++ wrapper and object model built around GGML.

The library provides C++ ownership, device discovery, backend management, execution contexts, tensors, graph construction, optimization, GGUF support, quantization, and related runtime helpers while remaining close to the upstream GGML API.

JOB GGML is structured as a collection of focused subsystems rather than a single monolithic interface. Each subsystem wraps a related area of GGML and can generally be used directly when only that functionality is required.

---

## Devices

The device subsystem discovers and represents the compute devices exposed by GGML.

`JobGgmlDeviceManager` is the primary facade for device discovery, state management, device lookup, implementation-specific indexes, and scheduler construction. Generic device wrappers expose common device properties, capabilities, interfaces, backends, and buffer types independent of the underlying implementation.

* [See also](devices.md)

---

## Device Implementations

JOB GGML provides backend-specific `JobGgmlDevice` implementations for supported GGML devices.

Current implementations include CPU, CUDA, Vulkan, OpenCL, BLAS, Hexagon, OpenVINO, SYCL, WebGPU, and zDNN. Backend-specific wrappers preserve the generic device interface while exposing functionality unique to that implementation, such as CPU feature detection, CUDA peer-to-peer operations, split buffers, and backend-specific memory helpers.

* [See also](impl_devices.md)

---

## Backends

The backend subsystem provides the JOB abstraction over GGML's execution, memory allocation, synchronization, and graph scheduling infrastructure.

Backend registries expose the backend implementations and devices registered with GGML, while backend instances provide the execution interface used to transfer tensor data, execute computation graphs, and synchronize work.

Memory management is represented through buffer types and buffers. A buffer type describes an allocation strategy associated with a backend device, while a backend buffer owns an actual allocation created from that type. Borrowed buffer views provide access to buffers whose lifetime remains controlled by another GGML object.

The subsystem also provides backend events for synchronization, reusable graph plans for backend-specific execution, graph copies for transferring complete graph state to a backend, and multi-backend schedulers for graph allocation, splitting, operation placement, and execution across multiple devices.

* [See also](backend.md)

---

## Contexts and Graphs

The context subsystem manages GGML initialization parameters, context ownership and lifetime, borrowed context views, and computation graphs.

It provides both owning and non-owning context wrappers, metadata sizing helpers, tensor creation through contexts, and graph construction through `JobGgmlCGraph`.

* [See also](context.md)

---

## Tensors

The tensor subsystem provides the C++ object model for inspecting and reasoning about native GGML tensors.

It includes tensor metadata, extents, physical layout, data and storage access, view relationships, semantic shapes, rank-specific tensor helpers, and operation metadata inspection through the `JobGgmlTensor` facade.

* [See also](tensors.md)

---

## Operations

The operations subsystem provides the C++ interface for constructing GGML tensor expressions and computation graphs.

`JobGgmlTensorOp` exposes unary, binary and multi-tensor operations, parameterized transformations and views, specialized model operations, and custom callback-backed operations. `JobGgmlTensorOpGraph` bridges composed tensor expressions into `JobGgmlCGraph` objects for execution.

* [See also](operations.md)

---

## Optimizer

The optimizer subsystem provides the C++ object model and high-level workflows for GGML optimization and training.

It includes optimizer configuration, AdamW and SGD parameters, datasets, optimizer schedules, results, optimization contexts, and training progress state. Native GGML epoch and fit operations are exposed directly, while JOB-managed workflows add stateful C++ callbacks, optimizer-step tracking, training and validation progress, and logical-batch gradient accumulation.

* [See also](optimizer.md)

---

## GGUF

The GGUF subsystem provides the model metadata and serialization layer used by JOB GGML.

It provides GGUF context management, typed key/value metadata, tensor metadata, type inspection, initialization controls, and file, memory, and callback-based I/O. `JobGguf` provides the primary facade for loading, inspecting, modifying, and writing GGUF content.

* [See also](gguf.md)

---

## Quantization

The quantization subsystem provides JOB wrappers around GGML quantization.

It includes quantization parameters, quantization results, and the quantizer interface used to convert model data between supported GGML quantization formats.

* [See also](quant.md)

---

## Miscellaneous Runtime Support

JOB GGML also includes shared runtime types and helpers used across the library.

This includes GGML enum mappings, type traits, thread-pool configuration, CPU thread-pool wrappers, and abort callback support.

* [See also](misc.md)

---