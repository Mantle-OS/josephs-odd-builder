# Job Ggml Tensors

The tensor subsystem provides the JOB GGML wrappers used to inspect and reason about native GGML tensors.

It separates tensor concerns into focused interfaces for shape and extents, physical memory layout, data and storage, view relationships, and operation metadata. `JobGgmlTensor` provides the primary facade over these interfaces while preserving access to the underlying native `ggml_tensor`.

The subsystem also provides rank-specific tensor wrappers for one-dimensional fibers, two-dimensional matrices, three-dimensional volumes, and four-dimensional batches, together with semantic shape types for common AI and numerical-computing tensor layouts.

Tensor wrappers are non-owning. Native tensors remain owned by their GGML context, and the owning context must outlive wrappers referring to those tensors.

---

## JobGgmlTensor

`JobGgmlTensor` is the primary facade for tensor inspection in JOB GGML. It wraps a borrowed native `ggml_tensor` and composes the focused tensor helpers used to inspect shape, layout, storage, view relationships, and operation metadata.

The native tensor remains owned by the GGML context in which it was created. `JobGgmlTensor` never releases the native tensor, so the owning context must outlive the wrapper and the inspection objects exposed through it.

The facade owns `JobGgmlTensorExtents`, `JobGgmlTensorView`, `JobGgmlTensorOperation`, `JobGgmlTensorData`, and `JobGgmlTensorLayout` objects for the same native tensor. These objects provide the detailed tensor API, while `JobGgmlTensor` exposes commonly used information directly as convenience shortcuts.

Tensor identity is exposed through its name, while shape information includes rank, per-dimension extents and strides, element count, byte size, padded byte size, and scalar through four-dimensional classification.

Layout shortcuts expose the current `JobGgmlTensorLayoutType` together with contiguous, contiguously allocated, transposed, permuted, and strided state. Shape, stride, and complete layout may also be compared directly with another `JobGgmlTensor`.

Type and storage inspection includes the tensor's `JobGgmlType`, type name, quantization state, backend-buffer association, host-buffer state, and raw data pointer. Tensor flags are exposed through convenience queries for input, output, parameter, loss, and compute tensors.

Operation metadata is available directly through the tensor facade, including the current `JobGgmlOp`, whether an operation is present, and the number of source tensors. View relationships can likewise be inspected through the view state, byte offset, immediate source, and root source.

The facade also exposes common shape-compatibility checks including repetition compatibility and matrix-multiplication compatibility.

For rank-specific inspection, a tensor may be viewed through `JobGgmlTensorFiber`, `JobGgmlTensorMatrix`, `JobGgmlTensorVolume`, or `JobGgmlTensorBatch`. These wrappers validate the tensor rank before exposing dimensional terminology specific to one-, two-, three-, or four-dimensional tensors.

Named two-dimensional tensors may also be created through the convenience factory helpers when a `JobGgmlContext` is available.

## JobGgmlTensorExtents

`JobGgmlTensorExtents` provides structural and dimensional inspection for a `JobGgmlTensor`. It wraps the extent and stride information stored in a native `ggml_tensor` while borrowing the tensor from its owning GGML context.

The wrapper exposes tensor rank, per-dimension extents and strides, total element count, volume, row count, byte size, and padded byte size. Convenience accessors are provided for GGML's four supported dimensions through the `ne0()` through `ne3()` and `nb0()` through `nb3()` interfaces, while complete extent and stride arrays may also be retrieved.

`JobGgmlTensorExtents` provides higher-level shape classification in addition to raw dimensional information. A tensor can be queried as a scalar, vector, matrix, three-dimensional tensor, or four-dimensional tensor, while separate compatibility queries expose whether its shape satisfies GGML's vector, matrix, or three-dimensional conventions.

Memory-layout information is also exposed through contiguity checks, including the dimension-specific GGML contiguity variants, together with transposed and permuted state.

Shape relationships between tensors can be evaluated directly. Extents may be compared for identical shape, checked for repeat compatibility, or tested for matrix-multiplication compatibility without requiring callers to inspect the native dimension arrays manually.

The underlying `ggml_tensor` is borrowed from the owning GGML context and is never released by `JobGgmlTensorExtents`. The native tensor therefore must outlive the extents wrapper.

## JobGgmlTensorView

`JobGgmlTensorView` provides inspection of the view relationship associated with a `JobGgmlTensor`. It wraps a borrowed native `ggml_tensor` and exposes the source, offset, ancestry, and depth of tensor views without taking ownership of the underlying tensor.

`isView()` reports whether the tensor is a GGML view. When a source is present, `source()` exposes the immediate source tensor and `offset()` reports the byte offset of the view into that source.

View chains may also be inspected recursively. `rootSource()` follows the `view_src` chain to the original source tensor, while `depth()` reports the number of view levels between the wrapped tensor and that root. `hasSource()` can be used to determine whether a specific tensor appears anywhere in the source chain.

Traversal is bounded internally to prevent malformed or cyclic native view chains from causing unbounded iteration.

The native tensor is borrowed from the owning GGML context and is never released by `JobGgmlTensorView`. The owning context and tensor must therefore outlive the view wrapper.

## JobGgmlTensorOperation

`JobGgmlTensorOperation` provides inspection of the operation metadata stored in a `JobGgmlTensor`. It wraps a borrowed native `ggml_tensor` and exposes the GGML operation, operation flags, source tensors, and encoded operation parameters without modifying or executing the tensor.

The current operation may be queried through the JOB `JobGgmlOp` enum or the native `ggml_op` value. The wrapper also exposes the human-readable operation name and symbol and provides helpers for determining whether the tensor has an operation or represents a specific operation type.

Operation dependencies are exposed through the tensor's source list. Callers can inspect individual source tensors, retrieve the complete fixed-size source array, determine how many sources are present, and test whether a particular tensor participates as a source.

GGML operation parameters are exposed as both individual `std::int32_t` values and the complete parameter array. The raw parameter storage may also be accessed through `parameterData()`. For structured parameters, `readParameter()` safely copies any trivially copyable type from a selected byte offset within GGML's operation-parameter storage.

Unary and GLU operations receive additional typed inspection through `JobGgmlUnaryOp` and `JobGgmlGluOp`, together with helpers that determine whether the wrapped tensor belongs to either operation family.

The native tensor is borrowed from the owning GGML context and is never released by `JobGgmlTensorOperation`.


## JobGgmlTensorData

`JobGgmlTensorData` provides access to a tensor's data type, storage association, flags, raw data pointer, and host-accessible values. It wraps a borrowed native `ggml_tensor` and does not own the underlying tensor or its storage.

The wrapper exposes the tensor's JOB and native GGML types together with the type name, block size, element type size, row size, quantization state, and `JobGgmlTypeTraits` information. These helpers provide the storage characteristics of the tensor without requiring direct use of GGML type functions. :contentReference[oaicite:1]{index=1}

Backend storage may be inspected through the tensor's associated native buffer and buffer type. Callers can determine whether a buffer is present and whether that buffer represents host-accessible memory. The raw tensor data pointer is also exposed, with a typed `float` pointer available for F32 tensors. :contentReference[oaicite:2]{index=2}

`JobGgmlTensorData` also manages the tensor flags used by GGML. Flags may be inspected, replaced, added, or removed, with convenience queries for input, output, parameter, loss, and compute tensors. Backend-specific extension storage is exposed through the tensor's `extra` pointer when present. :contentReference[oaicite:3]{index=3}

Direct scalar access is provided for signed 32-bit integers and 32-bit floating-point values using either linear indexes or four-dimensional coordinates. Entire tensors may also be filled with an integer or floating-point value. These operations use the host-side helpers provided by `ggml-cpu.h` and require tensor storage that is directly accessible from the host.

For CUDA, Vulkan, or other device-managed storage, callers should use `JobGgmlBackend` tensor transfer operations rather than the direct value-access helpers. Indexes and coordinates are validated before native GGML access, and invalid or unsupported access is reported through exceptions. :contentReference[oaicite:4]{index=4}

The native tensor is borrowed from the owning GGML context and is never released by `JobGgmlTensorData`.

## JobGgmlTensorRank

`JobGgmlTensorRank` is a compile-time rank wrapper for native GGML tensors. The template parameter fixes the expected tensor rank from one through `GGML_MAX_DIMS`, and construction verifies that the wrapped tensor matches that rank.

The wrapper borrows the native `ggml_tensor` from its owning GGML context and owns a `JobGgmlTensorExtents` helper used for dimensional inspection. It exposes rank-limited extent and stride access together with common tensor measurements including volume, element count, byte size, and padded byte size.

Layout information is also available through contiguity, transposed, and permuted-state queries. The underlying `JobGgmlTensorExtents` object remains accessible when more detailed dimensional inspection is required.

`FiberRank`, `MatrixRank`, `VolumeRank`, and `BatchRank` provide aliases for the one-, two-, three-, and four-dimensional template specializations. Higher-level tensor classes build on the same rank model while adding behavior specific to their dimensional form.

The native tensor is borrowed and is never released by `JobGgmlTensorRank`.

## JobGgmlTensorFiber

`JobGgmlTensorFiber` is the one-dimensional tensor specialization built on `FiberRank`. Construction requires a rank-one GGML tensor and preserves the common rank, extent, size, and layout inspection provided by `JobGgmlTensorRank`.

The wrapper exposes the fiber length, element stride, and whether the tensor is empty.

The native tensor is borrowed from the owning GGML context and is never released by `JobGgmlTensorFiber`.


## JobGgmlTensorMatrix

`JobGgmlTensorMatrix` is the two-dimensional tensor specialization built on `MatrixRank`. Construction requires a rank-two GGML tensor and preserves the common rank, extent, size, and layout inspection provided by `JobGgmlTensorRank`.

The wrapper exposes the matrix row and column counts together with element and row strides. It also provides convenience queries for square matrices and empty tensors.

The native tensor is borrowed from the owning GGML context and is never released by `JobGgmlTensorMatrix`.


## JobGgmlTensorVolume

`JobGgmlTensorVolume` is the three-dimensional tensor specialization built on `VolumeRank`. Construction requires a rank-three GGML tensor and preserves the common rank, extent, size, and layout inspection provided by `JobGgmlTensorRank`.

The wrapper exposes width, height, and depth together with row, column, and plane counts. It also provides element, row, and plane strides and can report the number of elements contained in each plane.

Convenience queries are provided for cubic volumes and empty tensors.

The native tensor is borrowed from the owning GGML context and is never released by `JobGgmlTensorVolume`.

## JobGgmlTensorBatch

`JobGgmlTensorBatch` is the four-dimensional tensor specialization built on `BatchRank`. Construction requires a rank-four GGML tensor and preserves the common rank, extent, size, and layout inspection provided by `JobGgmlTensorRank`.

The wrapper exposes width, height, depth, and batch count together with row, column, and plane counts. It also provides element, row, plane, and batch strides for inspecting the complete four-dimensional memory layout.

Convenience helpers report the number of elements per plane and per batch, whether the tensor contains a single batch, and whether it is empty.

The native tensor is borrowed from the owning GGML context and is never released by `JobGgmlTensorBatch`.

## Semantic Tensor Shapes

JOB GGML provides semantic shape structures for common tensor layouts used by language, attention, linear, and image models.

These structures describe the logical meaning of tensor dimensions rather than GGML's physical dimension ordering. For example, a semantic `[B, S, D]` tensor may be represented internally by GGML as `[D, S, B]`. Conversion between semantic and native ordering belongs to `JobGgmlTensorExtents` and `JobGgmlTensorLayout`.

`JobGgmlBSShape` represents `[B, S]` data such as token IDs, attention masks, and position IDs. `JobGgmlBSDShape` represents `[B, S, D]` hidden states, embeddings, and projected activations.

Attention-specific shapes include `JobGgmlBSHDShape` for `[B, S, H, Dh]`, `JobGgmlBHSDShape` for the transposed `[B, H, S, Dh]` form commonly used during attention computation, and `JobGgmlBHSSShape` for `[B, H, Sq, Sk]` attention score matrices.

`JobGgmlVDShape` represents vocabulary and embedding tables using `[V, D]`, while `JobGgmlLinearShape` represents linear-layer weights using `[Dout, Din]`.

For image and spatial workloads, `JobGgmlBCHWShape` represents `[B, C, H, W]` tensors such as image batches, convolution feature maps, and diffusion latents.

Each semantic shape provides validity checking, element-count helpers, equality comparison, and shape-specific derived information such as model dimension, spatial size, elements per batch, or square-attention state where appropriate.

## JobGgmlTensorLayout

`JobGgmlTensorLayout` provides physical memory-layout inspection for a `JobGgmlTensor`. It wraps a borrowed native `ggml_tensor` and builds on `JobGgmlTensorExtents` to describe how the tensor's logical dimensions are arranged in storage.

The wrapper classifies a tensor as contiguous, transposed, permuted, or generally strided through `JobGgmlTensorLayoutType`. It also exposes GGML's partial contiguity checks, allowing callers to distinguish full contiguity from layouts where only higher dimensions remain contiguous.

Physical allocation and logical traversal are treated separately. `isContiguouslyAllocated()` reports whether the tensor occupies one gap-free allocation even when its logical dimension order has been permuted, while `hasStorageGaps()` identifies layouts whose physical storage contains gaps.

Specialized layout queries expose contiguous rows and channels, transposed and permuted state, and trailing alignment padding. `trailingPaddingBytes()` reports padding added after the logical tensor storage without treating internal row or plane gaps as trailing padding.

The wrapper also exposes rank, extents, and strides through its owned `JobGgmlTensorExtents` helper. Singleton dimensions can be identified and counted, which is useful when reasoning about broadcasting and repetition without claiming that a particular operation is automatically compatible.

Two tensor layouts may be compared by shape, stride, or both. `hasSameLayout()` requires both identical shape and identical stride information.

The native tensor is borrowed from the owning GGML context and is never released by `JobGgmlTensorLayout`.
































