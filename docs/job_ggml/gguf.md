# GGUF

The GGUF subsystem provides the JOB GGML interface for reading, inspecting, writing, and working with GGUF model files and metadata.

It wraps GGUF initialization, metadata key/value access, tensor information, type traits, reader and writer workflows, and the associated GGML context used when tensor structures are materialized.

---

## JobGguf

`JobGguf` is the primary facade for the JOB GGUF subsystem. It owns the initialization parameters, GGUF context, reader, and writer objects used to load, inspect, modify, and serialize GGUF content.

The facade keeps a stable `JobGgufContext` wrapper shared by its reader and writer. Reading and reset operations replace only the native `gguf_context` owned by that wrapper, preserving the wrapper address borrowed by the I/O objects. :contentReference[oaicite:1]{index=1}

### Reading

GGUF content may be opened from a filesystem path, an existing `std::FILE`, a raw memory buffer, a byte span, or a caller-provided read callback.

All read operations use the owned `JobGgufInitParams` configuration and delegate parsing to `JobGgufReader`. Reader errors are adopted by the facade so callers can use a single error interface. :contentReference[oaicite:2]{index=2}

When a `JobGgmlContext::UPtr` output destination is configured, the initialization parameters may request creation of the associated GGML context during parsing.

### Writing

The current GGUF context may be saved to a filesystem path or an existing `std::FILE`, with optional metadata-only output.

The facade also exposes metadata serialization directly through `metadataSize()`, `metadata()`, and `writeMetadata()`, forwarding those operations to the owned `JobGgufWriter`. :contentReference[oaicite:3]{index=3}

### Context Inspection

Common GGUF context information is exposed directly through the facade, including format version, alignment, tensor-data offset, metadata entry count, and tensor count.

Metadata keys and tensors can be tested for existence, while individual or complete key/value entries may be retrieved as `JobGgufKv` objects. :contentReference[oaicite:4]{index=4}

### Context Mutation

The facade exposes the common mutation operations provided by `JobGgufContext`.

Metadata entries may be added, copied from another context, or removed. Tensors may be added from `JobGgmlTensor` objects, and existing tensor metadata or data may be updated through the facade. :contentReference[oaicite:5]{index=5}

### State and Errors

`reset()` replaces the current native GGUF context with a newly created empty context while preserving the facade's owned wrapper objects and their relationships.

Reader and writer errors are surfaced through the facade's `errorString()` and `hasError()` interface. `clearError()` clears the facade and both underlying I/O error states. :contentReference[oaicite:6]{index=6}


## JobGgufInitParams

`JobGgufInitParams` is the JOB representation of `gguf_init_params`. It controls how a GGUF reader initializes tensor metadata and whether an associated `JobGgmlContext` should be created while parsing.

The `noAlloc` setting controls whether tensor data storage is allocated by GGML during initialization. The object may also request context creation through `createContext()`.

When context creation is requested, the caller may provide a destination `JobGgmlContext::UPtr` through `contextOutput()`. This destination is borrowed by the initialization object; ownership of the resulting context is transferred into the caller-provided smart pointer after parsing.

The initialization state may be constructed from individual JOB values or from an existing native `gguf_init_params`. `setParams()` replaces the stored configuration, while `resetParams()` restores the default state.

`params()` converts the JOB-side configuration back to native `gguf_init_params` form and accepts the native `ggml_context **` destination required by the GGUF API.

## JobGgufTensorInfo

`JobGgufTensorInfo` represents the metadata associated with a tensor stored in a GGUF file.

Unlike `JobGgmlTensor`, which wraps a borrowed native tensor owned by a GGML context, `JobGgufTensorInfo` stores its own copy of the native `ggml_tensor` metadata together with the tensor's byte offset within the GGUF tensor data region.

The wrapper exposes the tensor name, type, rank, extents, strides, element count, byte size, and quantization state. Tensor type may be changed through either the JOB `JobGgmlType` enum or the native `ggml_type` representation.

Changing the tensor type recalculates the native stride information using the selected GGML type size and block size. Quantized types therefore require the first tensor extent to be divisible by the corresponding GGML block size.

The GGUF data offset may be queried or changed independently from the copied tensor metadata. `isAligned()` checks whether the stored offset satisfies a requested power-of-two alignment, while `paddedByteCount()` calculates the tensor storage size rounded up to that alignment.

`setTensor()` replaces the stored tensor metadata from a valid `JobGgmlTensor`, while `reset()` clears both the tensor metadata and file offset.

## JobGgufTypeTraits

`JobGgufTypeTraits` provides the JOB type-inspection layer for GGUF metadata values.

At compile time, `JobGgufTypeMap` maps supported C++ scalar types to their corresponding `JobGgufType`. Supported mappings include fixed-width signed and unsigned integers, `float`, `double`, `bool`, and `std::string`. The `JobGgufValueType` concept and associated type traits can be used to constrain templates to values that have a valid scalar GGUF representation.

GGUF arrays are treated separately from their element types. `GGUF_TYPE_ARRAY` represents the container itself, while the array element type must still be one of the supported scalar GGUF value types.

At runtime, `JobGgufTypeTraits` wraps either a JOB `JobGgufType` or native `gguf_type` and exposes the type name, storage size, and general type category.

Types may be classified as signed integers, unsigned integers, floating-point values, booleans, strings, or arrays. `isValueType()` identifies non-array GGUF values, while `isArrayElementType()` identifies types that may legally appear inside a GGUF array.

Compile-time helpers such as `typeFor<T>()`, `ggufTypeFor<T>()`, and `supportsType<T>()` provide direct mapping between C++ types and the GGUF metadata type system, while `isType<T>()` compares the current runtime type with a supported C++ value type.

Conversion helpers are also provided between `JobGgufType` and the native `gguf_type` representation.

## JobGgufKv

`JobGgufKv` represents an owned GGUF metadata key/value entry.

Each entry contains a non-empty string key and either a scalar value or an array of values. Supported value types are constrained by `JobGgufValueType`, allowing GGUF metadata to be constructed and accessed using the corresponding C++ scalar types.

The object owns its value storage. Fixed-width numeric and boolean values are stored as raw byte data, while strings are stored separately as `std::string` objects. Arrays use the same underlying element representation while tracking that the value is an array.

### Value and Serialized Types

`type()` reports the type of the stored value or array element. For example, both a scalar `std::uint32_t` and an array of `std::uint32_t` values report `JobGgufType::UInt32`.

`serializedType()` reports the outer type written to GGUF metadata. Scalar values report their normal value type, while arrays report `JobGgufType::Array`.

Conceptually:

    scalar uint32
        type()           -> UInt32
        serializedType() -> UInt32

    array<uint32>
        type()           -> UInt32
        serializedType() -> Array

This distinction preserves the GGUF representation in which an array has both a container type and a separate element type.

### Typed Access

Scalar values may be retrieved with `value<T>()`, while individual scalar or array elements may be retrieved with `value<T>(index)`.

Complete arrays may be returned with `values<T>()`.

All typed access is checked against the stored GGUF element type. Requests using an incompatible C++ type are rejected rather than implicitly converted.

### Typed Mutation

`setValue()` replaces the current value with a scalar, while `setValues()` replaces it with an array.

The C++ value type determines the corresponding GGUF element type through `JobGgufTypeTraits::typeFor<T>()`.

Fixed-width values are copied into owned byte storage. Boolean values use one byte per element, while strings are retained as owned string objects.

GGUF arrays may be empty. Scalars always contain exactly one value.

### Type Inspection

`JobGgufKv` exposes the stored element type through both `JobGgufType` and native `gguf_type` representations.

Convenience inspection methods identify strings, booleans, integers, and floating-point values. `typeTraits()` provides a `JobGgufTypeTraits` object for more detailed inspection of the element type.

### Casting

`cast()` reinterprets existing fixed-width byte storage using another fixed-width GGUF type.

Casting does not numerically convert the stored values. Instead, it changes how the existing bytes are interpreted. The byte storage must therefore be evenly divisible by the requested type size.

For scalar values, the storage must contain exactly one value of the requested type. Arrays may change their apparent element count as a result of reinterpretation.

String storage cannot be cast to fixed-width storage, and fixed-width storage cannot be cast to `String` or `Array`.

### Serializer Storage

`data()` exposes the owned byte representation used for fixed-width values, while `stringData()` exposes string storage.

These interfaces allow GGUF readers and writers to operate on the stored representation without requiring callers to manage the underlying value memory.

`reset()` clears the key and value storage and returns the object to an invalid empty state.


## JobGgufReader

`JobGgufReader` provides the input side of the JOB GGUF I/O interface. It parses GGUF data into a borrowed `JobGgufContext` and optionally produces an associated `JobGgmlContext` according to the supplied `JobGgufInitParams`.

GGUF data may be read from a filesystem path, an existing `std::FILE`, a raw memory buffer, a `std::span<const std::byte>`, or a caller-provided random-access read callback.

File-based input validates that the requested path exists and references a regular file before passing it to GGUF. Memory-based input rejects null or empty storage, while span input provides the corresponding C++ container-friendly interface.

### Callback Input

Callback-based reading allows GGUF data to come from storage that is not represented directly by a local file or contiguous memory buffer.

The callback receives an output buffer, absolute byte offset, and requested length and returns the number of bytes actually read. The caller also supplies the maximum chunk size and maximum expected input size used by the native GGUF callback interface.

The callback boundary is protected from C++ exceptions. Exceptions thrown by the caller are converted into reader errors rather than being allowed to cross the native GGUF callback ABI. Returning more bytes than requested is also treated as an error.

### Context Creation

`JobGgufInitParams` controls whether GGUF should also create a native GGML context while parsing.

When context creation is enabled, the reader temporarily owns the native `ggml_context` returned by GGUF, validates the requested output destination, wraps the context in `JobGgmlContext`, and transfers ownership into the caller-provided `JobGgmlContext::UPtr`.

The parsed native `gguf_context` is likewise kept under temporary ownership until the complete read operation has succeeded.

Only after all requested results have been validated and wrapped does the reader replace the native context stored by the destination `JobGgufContext`. This commit-at-the-end behavior preserves the previously loaded GGUF context when a later part of parsing or context construction fails.

### Error Handling

Reader failures are reported through `errorString()` and `hasError()`. Each read begins by clearing the previous error state, and successful completion clears any temporary parsing error.

Invalid input, native GGUF parsing failures, callback failures, context ownership inconsistencies, and wrapper-construction failures are reported without partially committing the new GGUF state.

## JobGgufWriter

`JobGgufWriter` provides the output side of the JOB GGUF I/O interface. It serializes a borrowed `JobGgufContext` to a file, an existing `std::FILE`, or a caller-provided metadata buffer.

The writer does not own the source `JobGgufContext`. The context must remain valid for the lifetime of the writer and while serialization operations are in progress.

### File Output

A complete GGUF context may be written to a filesystem path or an existing `std::FILE`.

The optional `metadataOnly` flag allows serialization to omit tensor data and write only the GGUF metadata representation.

Filesystem output validates the destination path and converts it to the native UTF-8 representation required by the GGUF API before writing.

### Metadata Serialization

The serialized metadata size may be queried through `metadataSize()` before allocating destination storage.

`metadata()` returns the complete serialized metadata as an owned `std::vector<std::byte>`.

Metadata may also be written directly into caller-owned storage through `writeMetadata()`. Both raw pointer/size and `std::span<std::byte>` interfaces are provided. The destination must be large enough to contain the complete metadata representation.

These metadata-only interfaces serialize the GGUF metadata block without requiring a file output path.

### Error Handling

Writer failures are reported through `errorString()` and `hasError()`.

Each write operation clears the previous error state before beginning. Invalid source contexts, invalid destinations, insufficient metadata buffers, and native GGUF write failures are reported without modifying the source context.




















