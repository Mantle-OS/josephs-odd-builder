# job::ai::comp (compilation)

> **"The math doesn't care about the silicon. The compiler does."**

The `comp` namespace is the muscle of the engine. It provides a unified, zero-overhead interface to CPU vector intrinsics, manages the "Reactor" (GEMM), and handles the non-linear forces (Activations).

It is built on a **Zero-Overhead Hardware Abstraction Layer (HAL)** that compiles down to raw assembly instructions.

---

## 1. The Hardware Handshake
Before code compiles, `check_hardware.cmake` inspects the silicon to tune the physics engine.

* **Flags:** Enforces `-ffast-math`, `-mavx2`, and `-mfma` for maximum throughput.
* **Cache Tuning:**
    * **64MB Mode:** Assumes 64MB L3 Cache (`JOB_BLOCK_SIZE_256`).
    * **Standard Mode:** Assumes 32MB L3 Cache.
* **Block Sizes:** Hard-codes tiling constants (e.g., `32x128`) to perfectly fit L1/L2 caches.

---

## 2. The Atoms: `simd_provider`
We do not write raw intrinsics. We write to the `SIMD` interface, which maps to the optimal backend at compile time.

* **Backends:** `AVX_F` (256-bit YMM) or `NEON_F` (128-bit).
* **Core Op:** `mul_plus` (Fused Multiply-Add). This is the heartbeat of the engine, performing $(A \times B) + C$ in a single cycle.

---

## 3. The Fuel: `NoiseTable`
Evolutionary strategies require billions of random numbers. Generating them at runtime is too slow.

* **Mechanism:** A 16MB/32MB buffer of pre-computed Gaussian noise pinned to the L3 cache.
* **Infinite Tape:** Uses bitwise masking to treat the buffer as an infinite loop of deterministic entropy.
* **Mutation:** `w += noise * sigma` is executed via vectorized FMA.

---

## 4. The Gymnast: `Transpose`
SIMD requires contiguous memory. `transpose.h` reorients data to optimize memory access patterns.

* **Kernel:** In-Register 8x8 Transpose.
* **Performance:** Swaps rows and columns of a 64-float block entirely within CPU registers using bitwise shuffles (`unpack`, `permute`), never touching L1 cache until the flip is complete.

---

## 5. The Alchemist: Activations
We trade negligible precision for massive speed using integer bit-hacks.

### The Functors (`activation_functors.h`)
* **Schraudolph's Exponential:** Approximates $e^x$ by manipulating IEEE 754 integer exponent bits. Used for fast Sigmoid/Swish.
* **Horner's Scheme:** Polynomial approximation for cases requiring smoother gradients.

### The Registry (`activation_types.h`)
A 1-byte enum identifier for all supported non-linearities, ranging from the standard (`ReLU`, `GELU`) to the biological (`GDN`, `Maxout`).

### The Dispatcher (`activate_parallel.h`)
* **Thresholding:** Tensors $<16k$ are processed on the calling thread.
* **Tiling:** Larger tensors are sliced into 4KB tiles and distributed to the thread pool.
* **Zero-Branching:** The switch statement happens *once per tile*, not once per element.

---

## 6. The Reactor: `GEMM`
The General Matrix Multiply kernel. The engine of the AI.

* **Micro-Kernel (8x8):** Accumulates a tile of C entirely in registers using Outer Product accumulation.
* **Tiling:** Loops over matrices in `JOB_BLOCK_SIZE` chunks to maintain L1 locality.
* **Parallelism:** Flattens the 2D tile grid into a 1D task list for the work-stealing scheduler.

---

## 7. The Synapse: `MLP`
Combines GEMM and Activations into neural structures.

* **Contract:** Zero-Allocation. Caller provides all scratch buffers (`HiddenBuf`).
* **Supported Architectures:**
    * Standard MLP ($W_2(\sigma(W_1 x))$).
    * Gated MLP / SwiGLU ($W_{proj}(\sigma(W_{gate} x) \odot (W_{val} x))$).
* **Fusion:** Supports fusing Residual Addition directly into the final projection.

---

## 8. The Lens: `FlashAttention`
Calculates the "Metric of the Portal" without $O(N^2)$ memory cost.

* **Tiling:** Computes attention scores in 16KB L1-sized blocks.
* **Online Softmax:** Rescales partial accumulators on-the-fly to ensure numerical stability without multiple passes.
* **Result:** Allows long-context inference on CPU.

---

## 9. The Collider: `AtomicMath`
The safety valve for Sparse Intelligence (MoE).

* **Mechanism:** Lock-free `atomicAdd` for floats using C++20 `std::atomic_ref`.
* **Usage:** Allows multiple experts to write to the same output buffer simultaneously without mutex contention or memory allocation for reduction buffers.

---
