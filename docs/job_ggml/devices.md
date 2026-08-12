# Devices

The JOB GGML device subsystem discovers, wraps, identifies, and exposes
the compute devices registered with GGML. The subsystem separates generic
device information and behavior from backend-specific implementations such
as CPU, CUDA, Vulkan, OpenCL, and other optional devices.

## JobGgmlDeviceManager

`JobGgmlDeviceManager` is the primary facade for device discovery and access in JOB GGML. It manages the lifetime of the canonical device collection, tracks the device subsystem state, and exposes discovered devices through generic and backend-specific interfaces.

A manager may scan for devices automatically when constructed or defer discovery until `scan()` is called. Its state is represented by `DeviceManagerState`, allowing callers to distinguish between uninitialized, scanning, ready, and error conditions. The manager also retains an error string when discovery cannot complete successfully.

Discovered devices are stored in a canonical UID-indexed collection of `JobGgmlDevice` objects. Devices can be accessed generically by UID or by index, while the CPU is exposed directly through `cpu()`. GPU devices that do not have a dedicated JOB implementation are retained in a fallback GPU index.

When optional backend support is compiled into JOB GGML, the manager also maintains implementation-specific indexes for devices such as CUDA, Vulkan, OpenCL, BLAS, Hexagon, OpenVINO, SYCL, WebGPU, and zDNN. These indexes provide typed access to the same discovered device objects rather than establishing a separate device ownership model.

`JobGgmlDeviceManager` also provides scheduler construction for discovered devices. A scheduler can be built from a device UID, directly from a `JobGgmlDevice`, or as a group of scheduler requests. Scheduler configuration includes graph size, parallel execution, and operation offload behavior. The most recently constructed scheduler may be accessed through `scheduler()` and released independently with `resetScheduler()`.

Calling `reset()` clears the manager's discovered-device state, backend indexes, scheduler state, and error information so that the device subsystem may be scanned again.

## JobGgmlDevice
`JobGgmlDevice` is the generic JOB wrapper for a single device reported by GGML. It provides the common device interface used by the device manager and by callers that do not require backend-specific functionality.

The native `ggml_backend_dev_t` is borrowed from the GGML registry and is not owned by `JobGgmlDevice`. The wrapper retains the JOB-side objects associated with that device, including its interface, properties, backend registry, backend instance, default buffer type, and optional host buffer type.

Device identity is exposed through `uid()`, which is derived from the device properties. The same properties object also provides access to device capabilities through `caps()`.

A `JobGgmlDevice` may expose an initialized `JobGgmlBackend` for execution and the buffer types used for device and host allocations. `hasBackend()` and `hasHostBufferType()` allow callers to determine which of these resources are available without inspecting the underlying pointers directly.

`JobGgmlDevice` is also the base class for backend-specific device implementations. Derived classes identify their implementation through `impl()` and may expose additional functionality while preserving the generic device interface used by `JobGgmlDeviceManager`.

## JobGgmlDeviceInterface

`JobGgmlDeviceInterface` is the low-level JOB interface to a native GGML device. It wraps a borrowed `ggml_backend_dev_t` and exposes the device operations used by the higher-level `JobGgmlDevice` class.

The interface provides direct access to device information reported by GGML, including its name, description, type, available memory, and property snapshot. A valid interface is simply one that holds a valid native device handle.

Backend resources may also be created through the interface. It can initialize a `JobGgmlBackend`, expose the device and host buffer types, and construct backend buffers over caller-owned host memory. Native resources created through these operations are wrapped in owning JOB objects while the underlying device handle remains borrowed from the GGML registry.

`JobGgmlDeviceInterface` also exposes device capability queries for tensor operations, buffer types, and operation offload. These checks allow callers to determine whether a device can support a requested execution or memory path before using it.

Device events may be created and synchronized through the same interface using `JobGgmlBackendEvent`, keeping device-level synchronization inside the JOB abstraction rather than requiring direct use of the GGML backend API.

## JobGgmlDeviceProps

`JobGgmlDeviceProps` is the JOB representation of the properties reported for a GGML device. It stores a local snapshot of `ggml_backend_dev_props` and exposes the device information through C++ types rather than requiring callers to inspect the native structure directly.

The property set includes the device type, name, description, device identifier, free memory, total memory, and device capabilities. Capabilities are represented by an owned `JobGgmlDeviceCaps` object so they can be accessed independently from the native GGML property structure.

`JobGgmlDeviceProps` is mutable. Individual values may be changed through their setters, while `setProps()` replaces the object state from a native `ggml_backend_dev_props`. The current JOB-side state can be converted back to the native representation through `props()`, and `resetProps()` restores the default property state.

Instances may also be compared for equality or inequality, allowing complete device-property snapshots to be compared as values.

## JobGgmlDeviceCaps

`JobGgmlDeviceCaps` represents the capability flags reported for a GGML device. It wraps `ggml_backend_dev_caps` and exposes the supported device features through a small C++ value object.

The capability set records whether the device supports asynchronous operation, host buffers, constructing backend buffers from caller-owned host pointers, and backend events. Individual capabilities may be inspected or changed through their corresponding getters and setters.

The complete capability state can be replaced from a native `ggml_backend_dev_caps`, converted back to the native representation, or reset to the default state. Capability objects may also be compared directly for equality or inequality.[paragraph describing actual capabilities]