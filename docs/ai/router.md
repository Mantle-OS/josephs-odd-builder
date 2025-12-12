# job::ai::router

> **"The Switchboard of the Mind."**

The `router` module determines how information flows through the sparse network. It decouples the *decision* of where to send data from the *execution* of that data.

## 1. The Taxonomy: `RouterType`
* **`TopK`:** The standard Learned Gating. A dense layer predicts which $K$ experts are best for a given token.
* **`Hash`:** Deterministic, fixed routing. Fast and requires no weights, but cannot "learn" specialization.
* **`Spatial`:** Manifold-based routing. Tokens are routed based on their coordinates in the semantic geometry (The Portal), effectively mapping specific regions of thought to specific experts (e.g., "The Crust" vs "The Core").
* **`State`:** Finite-State Machine routing. Useful for sequence-dependent logic (e.g., "If we are in the header, use Parser Expert; if in the body, use Semantic Expert").

## 2. The Traffic Laws: `LoadBalanceStrategy`
* **`TokenDropping`:** Hard constraints. If an expert's buffer is full, excess tokens are discarded. Guarantees memory limits and latency but risks information loss.
* **`Overflow`:** Soft constraints. Excess tokens are spilled to a secondary or shared expert.
* **`AuxLoss`:** (Training) Adds a penalty term to the loss function to encourage the router to distribute work evenly.

---

## 3. The Definition: `ExpertConfig`
Standard MoEs assume all experts are identical Feed-Forward Networks. `job_ai` supports **Heterogeneous Experts**.

* **`adapter`:** Each expert defines its own compute kernel via `AdapterType`.
    * *Example:* Expert A runs `Dense` (Logic), while Expert B runs `FMM` (Gravity/Physics).
* **`id`:** Unique identifier for routing tables.

## 4. The Rules: `RouterConfig`
Configures the global behavior of the dispatch system.

* **`type`:** The algorithm used to route tokens (TopK, Hash, Spatial).
* **`spatialRadius`:** (Spatial Router) Defines the "catchment area" in the semantic manifold. Tokens falling within this radius trigger the associated expert.
* **`loadBalance`:** Strategy for handling congestion (Token Dropping vs Overflow).
* **Presets:**
    * `TopK(N, k=2)`: Standard MoE setup. Routes to the 2 best experts.
    * `Hash(N)`: Deterministic, non-learned routing. Fast and statistically balanced.

---

## 5. The Manifest: `RouterPlan`
The data structure that decouples the **Decision** from the **Execution**.

* **`RouterToken`:** A dispatch ticket containing:
    * **Source:** The batch index (Row).
    * **Destination:** The Expert ID.
    * **Weight:** The gating signal strength (used for weighted averaging of outputs).
    * **Kernel:** The `AdapterType` (Dense, Flash, FMM), allowing per-token compute heterogeneity.
* **`RouterPlan`:** A lightweight view over a buffer of `RouterTokens`. It represents the complete set of instructions for the MoE layer for the current batch.

---

## 6. The Execution: `router_impl`
Implements the dispatch logic.

* **Zero-Allocation Top-K:**
    * Uses a stack-based Min-Heap to find the $K$ largest logits.
    * Avoids sorting the entire expert list ($O(N \log N)$) in favor of partial selection ($O(N \log K)$).
* **Deterministic Hashing:**
    * Samples a subset of input dimensions (strided) to compute a stable hash.
    * Ensures that the same token always routes to the same expert, even without learned weights.
* **Adapter Tagging:**
    * Every `RouterToken` is tagged with the target expert's `AdapterType` (e.g., Dense vs. Flash), enabling the MoE layer to dispatch to different compute kernels dynamically.

---

