# Quantization

The quantization subsystem provides the JOB GGML interface for converting model tensor data between GGML quantization formats.

It separates quantization configuration, execution, and result information into dedicated C++ objects. `JobGgmlQuantizer` provides the primary quantization interface, while `JobGgmlQuantizationParams` describes the requested conversion and `JobGgmlQuantizationResult` reports the resulting quantization state.

---

## JobGgmlQuantizer

`JobGgmlQuantizer` provides the quantization execution interface for JOB GGML.

The class exposes GGML quantization initialization and cleanup, quantized-type inspection, block and row sizing helpers, validation of quantization requests, and chunk-based quantization into caller-provided destination storage.

### Quantization Initialization

`init()` initializes GGML quantization support for a selected `JobGgmlType`.

`nativeFree()` releases the corresponding native GGML quantization state.

These operations manage upstream GGML quantization infrastructure rather than per-object state.

### Type Inspection

`isQuantized()` reports whether a `JobGgmlType` represents a quantized GGML storage type.

`requiresImportanceMatrix()` reports whether the selected quantization type requires an importance matrix.

`blockSize()` and `typeSize()` expose the native GGML block and storage sizes associated with a type.

`rowSize()` calculates the encoded byte size of one row for a selected type and element count. The row width must be positive and divisible by the type's GGML block size.

### Destination Sizing

`requiredBytes()` calculates the number of destination bytes required for the rows described by `JobGgmlQuantizationParams`.

The calculation validates row dimensions and protects against integer overflow before returning the required size.

### Validation

`validate()` verifies that the complete quantization request can safely be executed.

Validation includes the requested source range, row count, row width, GGML block-size alignment, row-boundary alignment, required importance-matrix presence, source capacity, destination offset, destination capacity, and arithmetic overflow.

The `start` value represents a source element offset and must begin on both a GGML block boundary and a complete row boundary.

Destination placement is derived from the corresponding starting row so chunked quantization may write directly into its correct position within a larger destination buffer.

### Chunk Quantization

`quantizeChunk()` quantizes the requested rows from a floating-point source span into caller-provided byte storage.

The request is validated before entering GGML. When an importance matrix is present, its borrowed data pointer is forwarded to the native quantizer.

On success, the method returns a `JobGgmlQuantizationResult` containing the target quantization type, the number of bytes written by GGML, and the number of rows processed.

## JobGgmlQuantizationResult

`JobGgmlQuantizationResult` describes the result of a JOB GGML quantization operation.

The object records the resulting `JobGgmlType`, the number of bytes written to the destination storage, and the number of rows processed during quantization.

`type()` reports the quantization type associated with the result.

`bytesWritten()` reports the amount of destination storage produced by the operation.

`rowsProcessed()` reports how many source rows were successfully processed.

The result object is copyable and movable and contains no owned external resources, making it suitable for returning quantization status by value.

## JobGgmlQuantizationParams

`JobGgmlQuantizationParams` describes the parameters used for a JOB GGML quantization operation.

The object stores the target `JobGgmlType` together with the region of tensor data to quantize. `start()` identifies the starting position, `rows()` identifies the number of rows to process, and `elementsPerRow()` describes the logical row width supplied to the quantizer.

The parameter object is copyable and movable, making it suitable for passing quantization configuration by value.

### Importance Matrix

An optional importance matrix may be supplied through `setImportanceMatrix()`.

The matrix is represented as a borrowed `std::span<const float>` and is not owned by `JobGgmlQuantizationParams`. The underlying storage must therefore remain valid for the duration of the quantization operation that consumes it.

`hasImportanceMatrix()` reports whether an importance matrix is currently present, while `clearImportanceMatrix()` removes the borrowed view.

### Configuration

The target quantization type and row-range information may be changed independently through their corresponding setters.

`JobGgmlQuantizationParams` does not itself perform quantization or own tensor storage. It only describes the requested conversion for use by `JobGgmlQuantizer`.





