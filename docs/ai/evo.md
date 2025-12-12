# job::ai::evo (evolution)

> **"Code that evolves code."**

The `evo` namespace manages the genetic lifecycle of the AI. It decouples the **Genotype** (the blueprint) from the **Phenotype** (the running network), enabling high-throughput evolutionary strategies (ES) and neuro-evolution (NEAT) without the overhead of object-oriented graph traversal.

---

## 1. The DNA: `Genome`
The Genome is a flat, serializable representation of a Neural Network, optimized for genetic operators.

### `LayerGene` (The Gene)
A struct that describes a single layer's topology.
* **Cache Aligned:** Explicitly padded to **32 bytes**. Exactly two genes fit in a standard 64-byte CPU cache line.
* **Relocatable:** Uses *offsets* into the weight buffer rather than pointers. A Genome can be `memcpy`'d to another machine or thread without serialization logic.

### `Genome` (The Chromosome)
* **Architecture:** A vector of `LayerGenes` describing the topology.
* **Weights:** A single contiguous `std::vector<float>` containing all parameters (weights + biases) for the entire network.
    * *Benefit:* Mutation and Crossover become $O(N)$ vector operations on a flat buffer, maximizing memory bandwidth.

---

## 2. The Agent of Chaos: `Mutator`
Manages the perturbation of Genomes using a deterministic Random Number Generator (RNG).

### Modes of Operation
1.  **Dense Mutation (Fast Path):**
    * **Trigger:** `weightMutationProb >= 1.0`.
    * **Mechanism:** Delegates to `comp::NoiseTable` (L3-pinned entropy).
    * **Performance:** Uses AVX2 Fused Multiply-Add (`mul_plus`) to perturb millions of weights in microseconds without generating new random numbers.
2.  **Sparse Mutation (Slow Path):**
    * **Trigger:** `weightMutationProb < 1.0`.
    * **Mechanism:** Iterates per-weight using scalar `std::normal_distribution` to apply sparse changes.

### Determinism
The Mutator holds its own `std::mt19937_64` state and can be explicitly seeded. This ensures that even "random" evolutionary runs are fully deterministic and replayable for debugging.

---

## 3. The Recombination: `Crossover`
Mechanisms for combining two parent Genomes into a child.

### Strategies
* **Uniform:** Stochastic mixing. Each gene is independently selected from Parent A or Parent B based on a probability (default 50%). Maximizes diversity.
* **Arithmetic:** Linear interpolation (`Child = αA + (1-α)B`). Uses vectorized math to blend parents, useful for fine-tuning in convex loss landscapes.

*Note: Currently requires Isomorphic Genomes (identical topology). Mismatched parents result in cloning the first parent.*

---

## 4. The Society: `Population`
A container for the current generation of individuals.

### Data Layout (SoA)
Genomes and Fitness scores are stored in separate vectors (`m_genomes`, `m_fitness`).
* **Ranking:** Sorting is performed on a lightweight list of *indices*, minimizing data movement of heavy Genome objects.

### Lifecycle (`evolveNextGeneration`)
1.  **Elitism:** The top $N$ individuals are copied unchanged to the next generation. This guarantees the best solution is never lost to destructive mutation.
2.  **Truncation Selection:** Parents are chosen uniformly at random from the **Top 50%** of the population. The bottom 50% are discarded.
3.  **Reproduction:** Winners breed (via Crossover) and their children are mutated to fill the remaining slots.

---

## 5. The Tribes: `Speciation` (NEAT)
*(Prototype Phase)*

To prevent premature convergence, `evo` protects novel structural mutations by grouping similar individuals into **Species**.

### The Distance Metric
Genomes are compared using the NEAT metric:
$$\delta = \frac{c_1 E}{N} + \frac{c_2 D}{N} + c_3 \cdot \bar{W}$$
* **Excess ($E$) / Disjoint ($D$):** Measures topological distance (structural differences).
* **Weight Difference ($\bar{W}$):** Measures parameter distance.

### Protection
Individuals compete primarily within their own Species. This allows a "weird" new topology (which may perform poorly at first) to optimize its own weights without being crushed by the dominant, optimized species.

---
