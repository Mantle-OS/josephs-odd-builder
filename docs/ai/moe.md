# `job::ai::moe`

> **"The Council of Experts."**

The `moe` namespace implements Sparse Mixture of Experts architectures. It wraps a collection of sub-layers (Experts) and a Dispatcher (Router) into a single module.

## 1. The Interface: `IMoE`
The abstract contract for all MoE implementations.

* **Heterogeneity:** Experts are added as generic `ILayer` pointers. This allows a single MoE layer to contain diverse sub-architectures (e.g., mixing Dense FFNs with Attention blocks).
* **Router Exposure:** The `route()` method is exposed publicly, allowing external inspectors to generate `RouterPlans` for analysis or visualization without executing the full forward pass.

---

## 2. The Types: `MoeType`
A lightweight enum mirroring `RouterType`, defining the dispatch strategy used by the MoE container.

* `Hash`: Deterministic.
* `TopK`: Learned Gating.
* `Spatial`: Geometric/Coordinate-based.
* `State`: FSM-based.

---

## 3. The Hive: `SparseMoE`
The container layer that executes the routing plan. It manages the lifecycle of sub-experts and handles the "Scatter-Gather" memory operations.

### Dynamic Memory Strategy
The layer inspects the `infer::Workspace` at runtime to choose the optimal reduction strategy:

1.  **Private Buffers (High Memory, High Speed):**
    * **Trigger:** If workspace has enough space to allocate a full `[Batch, Dim]` buffer for *every* active expert.
    * **Execution:** Experts write to private scratchpads in parallel. No locks required.
    * **Reduction:** A final parallel loop sums the private buffers into the output.
2.  **Atomic Reduction (Low Memory):**
    * **Trigger:** If workspace is constrained.
    * **Execution:** Experts write directly to the shared output buffer.
    * **Safety:** Uses `comp::atomicAdd` (CAS loops) to handle collisions when multiple experts contribute to the same token.

### Introspection
Implements `IParamGroup` recursively. It exposes its own gating weights (`gate`) and iterates through all sub-experts to build a hierarchical map of the entire sparse parameter space (e.g., `expert_5.attn.wq`).

---
