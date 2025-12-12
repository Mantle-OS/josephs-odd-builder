# job::ai::layers

> **"The Organs of the Organism."**

While `comp` handles the math and `evo` handles the DNA, `layers` defines the functional units that process information. It strictly separates **Architecture** (State) from **Execution** (Forward Pass).

---

## 1. The Contract: `ILayer`
The abstract base class that all neural modules must implement.

* **Static Analysis:** `getOutputShape()` allows the engine to pre-calculate memory requirements for the entire graph before inference begins.
* **Zero-Allocation:** The `forward()` method requires an `infer::Workspace`. Layers are forbidden from allocating heap memory themselves during inference; they must request scratch space from the workspace.
* **Introspection:** All layers implement `IParamGroup`, exposing a semantic map of their weights (e.g., "attn.wq", "ffn.bias") to optimizers and debuggers.

---

## 2. The Arena: `Workspace`
A static, pre-allocated memory arena used for all intermediate tensor storage.

* **Ping-Pong Buffering:**
    * The workspace is divided into `BufferA` and `BufferB`.
    * Sequential layers toggle between these buffers to avoid allocation overhead.
* **Alignment:** Uses `AlignedAllocator` (64-byte) to ensure compatibility with `comp` AVX kernels.
* **Persistence:** The arena grows to fit the largest model and never shrinks, preventing heap fragmentation.

---

## 3. The Components

### The Taxonomy (`LayerType`)
A 1-byte enum defining the supported blocks:
* **Core:** `Dense`, `Embedding`, `Residual`.
* **Transformer:** `Attention`, `RMSNorm`, `LayerNorm`.
* **Sparse:** `SparseMoE` (Mixture of Experts).

### The Workhorse (`Dense`)
The standard linear layer ($y = \sigma(xW + b)$).
* **Storage:** Weights and Biases are packed into a single contiguous aligned buffer.
* **Hybrid Scheduling:** Small layers run serially (fused GEMM+Bias+Act); large layers dispatch to the thread pool.
* **SIMD Bias:** Uses vectorized intrinsics to broadcast bias additions.

### The Eye (`Attention`)
Implements Multi-Head Attention using the **Adapter Pattern**.
* **Delegation:** The layer manages projections ($W_Q, W_K, W_V$); the actual attention mechanism is delegated to an `IAdapter` (FlashAttention, Barnes-Hut, etc.).
* **Flexibility:** Supports both Batched Training inputs `[B, S, D]` and Flattened Inference inputs `[Tokens, D]`.

---

## 4. The Builder: `LayerFactory`
Hydrates a flat `Genome` into live `ILayer` objects.

* **Interpretation:** Overloads `LayerGene` fields based on context (e.g., reusing `auxiliaryData` to store `AdapterType` or `TopK`).
* **Hydration:** Performs the critical copy from the Evolutionary storage (flat vector) to the Compute storage (aligned vector).
* **Safety:** Validates gene offsets against the genome size to prevent memory violations from corrupted DNA.

---
