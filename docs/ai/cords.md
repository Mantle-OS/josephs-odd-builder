# `job::ai::cords`

> **"Memory is flat. Intelligence is geometric."**

The `cords` (Coordinates) namespace is the **Physics Engine of Memory** for `job_ai`. 
It defines how raw bytes are aligned, shaped, viewed, and sliced without incurring copy 
overhead.

It is built on three strict laws:

1.  **Alignment:** All memory is 64-byte aligned (AVX-512 ready).
2.  **Locality:** Shapes (`Extents`) live on the stack; Views slice without copying.
3.  **Tiling:** Math is blocked at compile-time to saturate CPU registers.

-----

## 1\. The Substrate: `AlignedAllocator`

Standard vectors break SIMD optimization because they do not guarantee alignment. `cords` enforces it.

  * **Law:** Alignment is **64 bytes** (Cache Line size).
  * **Effect:** Prevents false sharing and enables AVX-512 / aligned AVX2 loads.
  * **Key Type:** `AiWeights` (A `std::vector` that plays nice with physics).

```cpp
using AiWeights = std::vector<float, AlignedAllocator<float, 64>>;
```

-----

## 2\. The Form: `Extents`

A tensor's shape is purely metadata. It should never trigger a heap allocation.

  * **Law:** Max Rank is 4 (Batch, Channel, Height, Width).
  * **Implementation:** `std::array` on the stack. `constexpr` everywhere.
  * **Performance:** Passing a Shape is as cheap as passing a struct.

```cpp
// A 4D shape defined entirely on the stack
constexpr Extents<4> shape(32, 3, 224, 224);
```

-----

## 3\. The Lens: `View` & `ViewIter`

We distinguish between **Owning** memory (`AiWeights`) and **Viewing** memory.

  * **Law:** A `View` is a lightweight window (Pointer + Shape + Strides).
  * **Mechanism:**
      * **Striding:** Row-Major by default for CPU prefetcher efficiency.
      * **Slicing:** `view.slice(i)` returns a sub-view (N-1 dimension) without copying data.
      * **Iteration:** `ViewIter` descends the dimensional ladder (4D -\> 3D -\> 2D -\> 1D).

-----

## 4\. The Semantics: Wrappers

To prevent "index soup," we strictly type our Views.

| Class | Rank | Role |
| :--- | :--- | :--- |
| **`Fiber`** | 1 | A vector/strand. Optimized for dot products. |
| **`Matrix`** | 2 | A table. Optimized for GEMM. |
| **`Volume`** | 3 | A 3D block. |
| **`Batch`** | 4 | The full NCHW stack. |

*Safety:* A function expecting a `Matrix` will fail to compile if passed a `Volume`.

-----

## 5\. The Blueprint: Tiles

These are compile-time constants that drive the math kernels in `comp`. They dictate register blocking.

  * **`Tile4` (SSE):** 4x4 block. Fits 128-bit registers.
  * **`Tile8` (AVX2 - The Golden Goose):** 8x8 block.
      * 8 floats = 256 bits (1 YMM Register).
      * Designed to saturate the 16 YMM registers of modern x86 CPUs (Ryzen/Intel Core).
  * **`Tile16` (AVX-512):** 16x16 block. Targeted for ZMM registers or L1 Cache blocking.

```cpp
// Example: The compiler uses this type to unroll loops 8x
comp::gemm<Tile8>(...); 
```

-----

### Usage Summary

```cpp
// 1. Allocate Raw Matter (Aligned)
AiWeights memory(1024); 

// 2. Impose Form (View as 32x32 Matrix)
Matrix mat(memory.data(), 32, 32);

// 3. Slice and Observe (Zero-Copy)
Fiber row = mat.slice(5); // View of the 5th row

// 4. Access (Safe)
float val = row.at(0);
```

-----
