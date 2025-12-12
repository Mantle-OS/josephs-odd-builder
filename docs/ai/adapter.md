# `job::ai::adapters`

> **"The Physics Cartridges."**

The `adapters` module implements the **Strategy Pattern** for compute kernels. It allows the `Attention` layer (or MoE Experts) to hot-swap the underlying mathematical model used to process token interactions.

## 1. The Context: `AdapterCtx`
A unified parameter struct passed to all adapters during the `adapt()` call.

* **Dynamics:** `dt` (Time step) for physics-based adapters (`Verlet`, `BarnesHut`).
* **Dimensions:** `embedDim` / `headDim` for standard neural adapters (`Flash`, `Dense`).
* **Flexibility:** Adapters are designed to consume only the fields relevant to their domain, ignoring the rest.

---

## 2. The Taxonomy: `AdapterType`
Defines the available compute kernels.

* **Neural Engines:**
    * `Dense`: Standard $O(N^2)$ attention using BLAS. Reference implementation.
    * `Flash`: Memory-efficient $O(N)$ attention using L1 tiling (from `comp/flash_attention.h`).
    * `LowRank`: LoRA / SVD approximations for efficient adaptation.
* **Physics Engines:**
    * `FMM` (Fast Multipole Method): Computes potentials/fields using multipole expansions. $O(N)$.
    * `BarnesHut`: Tree-based gravity approximation. $O(N \log N)$.
    * `Verlet`: Time-integration for particle dynamics.

---

## 3\. The Interface: `IAdapter`

The abstract contract that unifies Neural Attention with Physical Interaction.

### The `adapt` Signature

```cpp
virtual void adapt(pool, shape, sources, targets, values, output, ctx)
```

### The Unification Table

The adapter interface redefines standard AI terms into generic interaction terms:

| Neural Term | Physics Term | Adapter Argument | Meaning |
| :--- | :--- | :--- | :--- |
| **Query ($Q$)** | **Target Position** | `targets` | The location/token *requesting* information. |
| **Key ($K$)** | **Source Position** | `sources` | The location/token *emitting* information. |
| **Value ($V$)** | **Charge / Mass** | `values` | The content or magnitude of the signal. |
| **Attention** | **Potential / Force** | `output` | The integrated result of the interaction. |

-----

## 4. The Neural Engine: `FlashAdapter`
The standard implementation for Transformer attention.

* **Mapping:**
    * `targets` $\rightarrow$ Queries ($Q$)
    * `sources` $\rightarrow$ Keys ($K$)
    * `values` $\rightarrow$ Values ($V$)
* **Kernel:** Delegates to `comp::flashAttentionForward`, utilizing L1-tiled matrix multiplication and online softmax for $O(N)$ memory usage.

---

## 5. The Linear Engine: `LowRankAdapter`
Implements $O(N)$ Linear Attention by exploiting the associativity of matrix multiplication.

* **Mechanism:** Computes $Q(K^T V)$ instead of $(QK^T)V$.
* **State Matrix:** Creates a $D \times D$ "Memory Matrix" ($K^T V$) that summarizes the entire sequence context.
* **Efficiency:** Decouples memory usage from sequence length. Ideal for "Mantle" experts handling massive, low-frequency contexts.

---

## 6. The Baseline: `DenseAdapter`
The reference implementation of standard Scaled Dot-Product Attention.

* **Complexity:** Time $O(N^2)$, Space $O(N^2)$.
* **Mechanism:**
    1.  Explicitly materializes the full $S \times S$ attention matrix (Scores).
    2.  Applies Softmax.
    3.  Multiplies by Value ($V$).
* **Role:** Serves as the ground truth for validating faster, approximate adapters (Flash, FMM, LowRank). Not intended for production use on long sequences due to quadratic memory costs.

---

## 7. The Simulator: `VerletAdapter`
Treats the input tokens as physical particles interacting via Newtonian forces.

* **Mapping:**
    * Input Embedding $D$:
        * $D_{[0..2]} \to$ Position ($\vec{r}$)
        * $D_{[3..5]} \to$ Velocity ($\vec{v}$)
        * $D_{[6]} \to$ Mass ($m$)
    * Remaining dimensions are carried forward passively.
* **Mechanism:**
    * Calculates N-Body forces (Gravity/Electrostatics) between all tokens.
    * Uses a **Symplectic Integrator** (Velocity Verlet) to advance the system by time $dt$.
* **Interpretation:** "Attention" becomes "Force". "Context" becomes "Phase Space". The layer output is the state of the conceptual system at time $t + dt$.

---

## 8. The Galaxy: `BhAdapter`
Approximates infinite-context attention using the **Barnes-Hut Tree Code**.

* **Mechanism:**
    * Maps embedding dimensions to 3D spatial coordinates.
    * Builds a recursive **Octree** of the token space.
    * Approximates distant interactions using **Multipole Expansion** (Center of Mass).
* **Complexity:** $O(N \log N)$ instead of $O(N^2)$.
* **Metaphor:** Semantic Gravity. Tokens cluster into topics; topics exert influence on other topics based on their aggregate "mass" (importance) and distance in the embedding manifold.

---

## 9. The Math Dragon: `FmmAdapter`
The Fast Multipole Method (FMM). The only attention mechanism that achieves true $O(N)$ scaling.

* **Mechanism:**
    * Projects embeddings to 3D.
    * Performs Multipole Expansion (Order $P=3$) to represent distant token clusters as complex fields (Monopole, Dipole, Quadrupole).
    * **Pipeline:**
        1.  **P2M:** Particles form Multipoles in leaf nodes.
        2.  **M2M:** Multipoles propagate up the tree (Aggregation).
        3.  **M2L:** Multipoles transfer to Local expansions (Interaction).
        4.  **L2L:** Local expansions propagate down.
        5.  **L2P:** Local expansions evaluate potential/force on particles.
* **Result:** Linear scaling. Enables effectively infinite context windows where the computational cost is proportional only to the number of tokens, not their interactions.

---

## 10. The Factory: makeAdapter
A lightweight dispatcher that hydrates an AdapterType enum into a live unique_ptr<IAdapter>.

---
