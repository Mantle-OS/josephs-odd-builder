The `job::ai::learn` namespace defines the objective functions for the Mantle-OS stack. It provides a standardized interface for scoring genomes across diverse tasks, from discrete logic to continuous physical control and linguistic modeling.

### The `ILearn` Interface: The Feedback Loop

Every learning environment implements a core simulation loop within the `learn(genome)` method:

1. **Flywheel Load**: The candidate `Genome` is loaded into the `infer::Runner`.
2. **Simulation**: The environment executes a sequence of trials (Inference + Physics).
3. **Fitness Calculation**: A scalar score is generated, typically normalized where $100\%$ represents perfect alignment with the target objective.
4. **Atomic Termination**: Uses `std::memory_order_acquire` to safely signal completion across the thread pool.

### High-Density Configuration (`LearnConfig`)

The `LearnConfig` is a 32-byte packed structure (verified via `static_assert`) that defines the world-parameters:

* **`corpus`**: The raw data bytes for linguistic or protocol-based learning.
* **`targetFitness`**: The exit-condition for the coach.
* **`maxSteps`**: The temporal boundary of the simulation (e.g., 500 steps of a balanced pole).
* **`initWsMb`**: Explicit control over the **Inference Workspace** size, ensuring memory-deterministic training.

---

## 2. Environment Types (`LearnType`)

| Environment | Purpose | Target Metric |
| --- | --- | --- |
| **`XOR`** | Discrete Logic | Binary classification accuracy. |
| **`CartPole`** | Physics Control | Time-to-failure in a continuous gravity sim. |
| **`Language`** | Semantic Modeling | Sequence prediction accuracy over a `corpus`. |
| **`Portal`** | Trajectory Alignment | Distance from a target "worldline" in latent space. |

### The "Singularity" Guard

To ensure the stability of the `Coach`, Mantle employs bit-level inspection of output data. The `punishLearner` utility identifies `NaN` or `Infinity` results at the hardware level, allowing the system to immediately discard "broken" genomes that have entered a numerical singularity.

---



