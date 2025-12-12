# job::ai::coach

> **"The Personal Trainer."**

The `coach` module orchestrates the learning process. It manages the `Population`, executes the `Evolution` loop, and interfaces with the `learner` to evaluate performance.

## 1. The Curriculum: `CoachType`
Defines the high-level strategy used to improve the population.

* **`Genetic`:** Standard Genetic Algorithm (GA). Relies on **Crossover** (recombination) and sparse mutation. Best for topological search and diverse exploration.
* **`ES` (Evolution Strategy):** Optimization via dense Gaussian perturbation. Relies on the Law of Large Numbers to estimate gradients. Best for parameter tuning (weight optimization).
* **`CMA_ES`:** (Reserved) Covariance Matrix Adaptation. A more advanced ES that adapts the mutation distribution shape.

---

## 2. The Contract: `ICoach`
The abstract interface for all optimization strategies.

* **Task Agnostic:**
    * Uses a `std::function` callback (`Evaluator`) to score Genomes. The Coach has no knowledge of the underlying problem domain.
* **Seeded Optimization:**
    * The `coach()` method accepts a `parent` Genome. The Coach bootstraps the population from this seed (e.g., by cloning and mutating), allowing for continuous learning or fine-tuning of existing models.
* **Dynamic Control:**
    * Exposes mutation rates and population sizing as runtime variables, enabling **Simulated Annealing** (reducing mutation rate over time) or dynamic scaling based on hardware load.

---

## 3. The Rules: `ESConfig`
Configuration for the Evolutionary Strategy engine.

* **`sigma`:** Controls the magnitude of Gaussian noise applied during mutation. Analogous to the Learning Rate in Gradient Descent.
* **`decay`:** The annealing factor. `sigma` is multiplied by this factor every generation, allowing the search to settle into fine-grained optimization as time passes.
* **`populationSize`:** The number of offspring generated per epoch.
* **Presets:**
    * `kFastTest`: Low latency, deterministic.
    * `kStandard`: Balanced for single-machine training.
    * `kDeepSearch`: High-throughput configuration for solving complex, non-convex problems (requires high core count).

---


This is the **God Loop**.

You have implemented a **(1, $\lambda$) Evolution Strategy**.
* **1:** You start with one parent (the best from the previous generation).
* **$\lambda$:** You generate `populationSize` mutants in parallel.
* **Selection:** The single best mutant becomes the parent for the next generation.

This is essentially **High-Dimensional Hill Climbing with a Shotgun.**

### The Fourth Stone: `ESCoach`

**The Analysis:**

1.  **The Parallelism:**
    * `threads::parallel_for(*m_pool, ...)`
    * This is where your architecture pays off. Because `Genome` is just data (flat vectors) and `Mutator` is seeded locally, there is **zero contention**. You can run this on 128 cores, and it will scale linearly.

2.  **The Seeding (`// This still does not seem global !`):**
    * `localMutator.seed(m_generation * popSize + i);`
    * **Verdict:** This **is** correct for deterministic reproducibility *within a single run*.
    * **The Nuance:** If you stop the program and restart it, `m_generation` resets to 0. You will repeat the exact same "random" mutations.
    * **The Fix (Later):** When you load a checkpoint, you must load the `generation` counter too. As long as `generation` keeps climbing, the seeds `(Gen * Size + i)` remain unique and deterministic.

3.  **The Annealing:**
    * `m_config.sigma *= m_config.decay;`
    * Simple and effective. As the generations pass, the "Explosion Radius" of the mutations gets smaller, allowing the AI to settle into the valley minima.

---

## 4. The Trainer: `ESCoach`
Implements the **(1, $\lambda$) Evolution Strategy**.

### The Algorithm
1.  **Replication:** The best genome from the previous generation (The Parent) is cloned $N$ times.
2.  **Mutation:** Each clone is perturbed by the `Mutator` using Gaussian noise scaled by $\sigma$ (Sigma).
    * **Parallelism:** This step is embarrassingly parallel. Each thread handles a subset of the population using a deterministically seeded RNG (`Generation_ID + Thread_ID`).
3.  **Evaluation:** (Going away) The `Evaluator` callback measures the fitness of each mutant.
4.  **Selection:** The mutant with the highest fitness becomes the Parent for the next generation.
5.  **Annealing:** $\sigma$ is multiplied by the decay factor to reduce search variance over time.

---




