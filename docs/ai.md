# Mantle / `job_ai` Ecosystem

`job_ai` is a vertically integrated, C++23 high-performance AI engine. It is designed to bypass the traditional 
deep learning stack in favor of hardware native execution, physics informed algorithms, and forward-only 
evolutionary learning.

## Core Infrastructure
### [Coordinates](docs/ai/cords.md)

The geometric and memory foundation of the stack. This module manages tensor shapes, extents, and the `AiWeights` 
container. It enforces **64-byte cache-line alignment** to ensure zero-copy compatibility with 
high-end SIMD instruction sets.

### [Computation](docs/ai/comp.md)
Contains hand-tuned, fused kernels for GEMM, FlashAttention, and 18+ activation types. 
It utilizes a unified Hardware Abstraction Layer to target **AVX2, AVX-512, and ARM NEON** at compile-time with 
zero runtime overhead.

### [Inference](docs/ai/infer.md)
The training or runtime loop owned by its "learner" type. 


---

## Intelligence & Learning

### [Evolution](docs/ai/evo.md)

The primary optimization engine. Implements **Forward-Only Evolutionary Synthesis**, utilizing high-performance Gaussian mutation and arithmetic crossover to navigate the weight 
manifold without the memory cost of backpropagation.

### [Training (Coach)](docs/ai/coach.md)

The high-level orchestrator for the learning process. 
It manages populations, annealing schedules, and the parallel evaluation of genomes across thread pools.

### [Specialized Learning Tasks](docs/ai/learn.md)

Implementations of specific fitness evaluators and environments, ranging from classic control problems (CartPole) 
to complex sequence alignment (Language Models).

---

## Specialized Architectures

### [Layers](docs/ai/layers.md)

The structural building blocks of a model, including Dense, MLP, and Attention variants. 
Layers define the topology while delegating heavy math to the Adapter layer or Activation type in the case of a Dense Layer

### [M.O.E (Mixture of Experts)](docs/ai/moe.md)


### [Routing](docs/ai/router.md)
The Router for the MOE layer


---

## Performance & Adapters

### [Adapters](docs/ai/adapter.md)

The backend selection layer. This allows `job_ai` to dynamically switch between **Dense**, **Flash**, and **Physics-Informed** (FMM, Barnes-Hut) execution strategies depending on sequence length and hardware constraints.

### [Tokens](https://www.google.com/search?q=docs/ai/token.md)

The encoding layer. Includes high-efficiency BPE, Motif, and Byte-Lattice tokenizers designed 
to map raw data into the particle-analogue space used by the attention adapters.
