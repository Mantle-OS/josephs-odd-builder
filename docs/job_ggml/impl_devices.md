# Device Implementations

The JOB GGML device implementation layer extends `JobGgmlDevice` with functionality that is specific to individual GGML backends. These classes preserve the common device interface while exposing backend-specific controls, capabilities, and diagnostics.

## JobGgmlCpu

`JobGgmlCpu` is the CPU-specific implementation of `JobGgmlDevice`. Construction requires the wrapped device to expose a valid GGML CPU backend, and the class identifies itself as `JobGgmlDeviceImpl::Cpu`.

In addition to the common device API, `JobGgmlCpu` exposes controls for the active CPU backend. Callers can configure the number of worker threads, attach or detach a `JobGgmlThreadPool`, install or remove a `JobGgmlAbortCallback`, and enable the GGML reference implementation. These operations are applied directly to the backend associated with the device.

CPU NUMA support is exposed through static initialization and query helpers. `initializeNuma()` selects the GGML NUMA strategy, while `isNuma()` reports whether NUMA support is active.

`JobGgmlCpu` also exposes the processor capabilities detected by GGML. These queries cover x86 features including SSE3, SSSE3, AVX, AVX-VNNI, AVX2, BMI2, F16C, FMA, AVX-512 variants, and AMX INT8; ARM features including NEON, FMA, FP16 vector arithmetic, dot product, integer matrix multiplication, SVE, and SME; and additional architecture support including RISC-V vector extensions, VSX, VXE, WebAssembly SIMD, and the llamafile execution path.

The CPU implementation also provides architecture-specific vector information where available, including the SVE count and RISC-V vector length. Its `dump()` implementation produces a compact diagnostic summary of NUMA state and the detected processor capabilities for use by higher-level diagnostics.


## JobGgmlCuda

`JobGgmlCuda` is the CUDA-specific implementation of `JobGgmlDevice`. It preserves the common device, backend, buffer, property, and capability interfaces while exposing CUDA functionality required for single- and multi-GPU execution. The class identifies itself as `JobGgmlDeviceImpl::Cuda`.

A CUDA device can report the number of CUDA devices visible through GGML and determine its own CUDA ordinal from the native GGML device name. The wrapper does not infer CUDA device identity from discovery order.

`JobGgmlCuda` exposes CUDA peer-to-peer support for systems containing multiple GPUs. Devices can be queried for peer accessibility, peer access can be enabled in one or both directions, and memory can be copied directly between peer-accessible devices. CUDA device selection is restored after peer-management operations so these helpers do not permanently change the caller's active CUDA device.

Existing caller-owned host memory may also be registered for CUDA access and later unregistered through the GGML CUDA backend helpers. Registration does not transfer ownership of the host memory to `JobGgmlCuda`.

For distributed CUDA workloads, `splitBufferType()` exposes GGML's CUDA split-buffer support. A main CUDA device and tensor split may be supplied to obtain the backend buffer type used for distributing tensor storage across devices.

`allReduceTensor()` provides a collective reduction path across matching collections of CUDA backends and tensors. The method validates each JOB wrapper, converts the collections to their native GGML representations, and forwards the operation to the GGML CUDA all-reduce implementation.

The CUDA implementation also provides a compact diagnostic `dump()` containing its CUDA index, validity and backend state, device identity, description, and reported free and total memory.

## JobGgmlVulkan

`JobGgmlVulkan` is the Vulkan-specific implementation of `JobGgmlDevice`. It preserves the common device, backend, buffer, property, and capability interfaces provided by the base class while identifying itself as `JobGgmlDeviceImpl::Vulkan`.

The wrapper can verify that its associated backend is a valid Vulkan backend through `isVulkanBackend()`. It also exposes the number of Vulkan devices reported by GGML through the static `deviceCount()` helper.

`dump()` provides Vulkan-specific diagnostic information for use by higher-level device reporting.

## JobGgmlBlas

`JobGgmlBlas` is the BLAS-specific implementation of `JobGgmlDevice`. It preserves the common device, backend, buffer, property, and capability interfaces provided by the base class while identifying itself as `JobGgmlDeviceImpl::Blas`.

The wrapper can verify that its associated backend is a valid BLAS backend through `isBlasBackend()`.

`dump()` provides BLAS-specific diagnostic information for use by higher-level device reporting.

## JobGgmlOpenCl

`JobGgmlOpenCl` is the OpenCL-specific implementation of `JobGgmlDevice`. It preserves the common device, backend, buffer, property, and capability interfaces provided by the base class while identifying itself as `JobGgmlDeviceImpl::OpenCl`.

The wrapper can verify that its associated backend is a valid OpenCL backend through `isOpenClBackend()`.

OpenCL functionality is limited by the support currently provided by upstream GGML. In particular, this should not be treated as a general-purpose OpenCL replacement for the CUDA backend; upstream OpenCL support currently targets a much narrower set of devices and environments.

`dump()` provides OpenCL-specific diagnostic information for use by higher-level device reporting.

## JobGgmlHexagon

`JobGgmlHexagon` is the Hexagon-specific implementation of `JobGgmlDevice`. It preserves the common device, backend, buffer, property, and capability interfaces provided by the base class while identifying itself as `JobGgmlDeviceImpl::Hexagon`.

The wrapper can verify that its associated backend is a valid Hexagon backend through `isHexagonBackend()`.

`dump()` provides Hexagon-specific diagnostic information for use by higher-level device reporting.

## JobGgmlOpenVino

`JobGgmlOpenVino` is the OpenVINO-specific implementation of `JobGgmlDevice`. It preserves the common device, backend, buffer, property, and capability interfaces provided by the base class while identifying itself as `JobGgmlDeviceImpl::OpenVino`.

The wrapper can verify that its associated backend is a valid OpenVINO backend through `isOpenVinoBackend()` and exposes the number of OpenVINO devices reported by GGML through `deviceCount()`.

OpenVINO-specific buffer helpers allow callers to determine whether a `JobGgmlBackendBuffer` belongs to the OpenVINO backend and whether a `JobGgmlBackendBufferType` represents an OpenVINO device or host buffer type. These checks operate on the existing JOB backend wrappers rather than requiring direct inspection of native GGML handles.

For OpenVINO buffers, `bufferContextId()` exposes the backend context identifier when available. Invalid buffers or buffers not owned by the OpenVINO backend return no value.

`dump()` provides OpenVINO-specific diagnostic information including wrapper validity, backend validity, device identity, description, and reported free and total memory.

## JobGgmlSycl

`JobGgmlSycl` is the SYCL-specific implementation of `JobGgmlDevice`. It preserves the common device, backend, buffer, property, and capability interfaces provided by the base class while identifying itself as `JobGgmlDeviceImpl::Sycl`.

The wrapper can verify that its associated backend is a valid SYCL backend and exposes the number of SYCL devices reported by GGML. It can also return the backend's GPU list, print the available SYCL devices, and query individual device descriptions and memory information.

For distributed workloads, `splitBufferType()` exposes GGML's SYCL split-buffer support. A tensor split can be supplied to obtain the backend buffer type used for distributing tensor storage across SYCL devices.

SYCL does not expose host-memory registration through this wrapper because the upstream backend does not support that path.

`dump()` provides SYCL-specific diagnostic information including wrapper validity, backend validity, device identity, description, and reported free and total memory.

## JobGgmlWebGpu

`JobGgmlWebGpu` is the WebGPU-specific implementation of `JobGgmlDevice`. It preserves the common device, backend, buffer, property, and capability interfaces provided by the base class while identifying itself as `JobGgmlDeviceImpl::WebGpu`.

The current wrapper does not expose additional WebGPU-specific controls beyond the generic device interface. Backend-specific functionality can be added as upstream GGML WebGPU support expands.

`dump()` provides WebGPU-specific diagnostic information for use by higher-level device reporting.


## JobGgmlZdnn

`JobGgmlZdnn` is the zDNN-specific implementation of `JobGgmlDevice`. It preserves the common device, backend, buffer, property, and capability interfaces provided by the base class while identifying itself as `JobGgmlDeviceImpl::Zdnn`.

The current wrapper does not expose additional zDNN-specific controls beyond the generic device interface. Upstream GGML contains internal zDNN backend helpers that are not currently part of the public `ggml-zdnn.h` API, so JOB GGML deliberately avoids depending on those non-public interfaces.

`dump()` provides zDNN-specific diagnostic information for use by higher-level device reporting.

## Unsupported Device Implementations

The following GGML device backends are currently not wrapped by JOB GGML:

- VirtGPU
- Metal
- ZenDNN
- CANN

The build structure already reserves implementation entries for these backends, but their JOB device wrappers are not currently enabled. They should not be considered supported until the corresponding `JobGgmlDevice` implementations are completed and added back to the build.
