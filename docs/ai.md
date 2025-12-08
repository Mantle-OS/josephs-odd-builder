# Mantle / `job_ai`

*Meh whatever .....*

### A Manifold of Future Time

> *“Where is your AI stack?”*
>
> *“Right here. On the CPU. With dragons slain and no GPU tax.”*


`job_ai` is an experimental C++ AI engine built for:

* **CPU-first** inference and training
* **Forward-only**, evolutionary learning (no backprop graph unless you absolutely insist)
* **Sparse, structured intelligence**: FMM attention, MoE, and fast IPC
* **Brutal simplicity** at the core: threads, math, and wires
* It’s not a PyTorch wrapper. It’s not “yet another deep learning framework.”
* Hand-rolled manifold where time only flows forward and gradients are optional guests.

---

## Benchmarks

[See: This document](docs/ai_benchmarks.md) or run the tests.

---

## Why this exists

Two taxes I didn’t want to pay *(I'm not stubborn not hard headed at all)*:

1. **The GPU Tax**

   * High VRAM, CUDA lock-in, and a hardware treadmill.
   * Great for giant models. Overkill for a tightly-optimized builder OS.

2. **The Algorithm Tax**

   * Full backpropagation requires storing the entire activation graph.
   * Time-symmetric, backward-in-time error signals that biology *doesn’t* use.
   * Physics does not run reverse-time gradient descent every time something falls.

So the question became:

> What would an AI stack look like
> if it cared first about **forward dynamics**, **evolution**, and **composition**,
> not about replaying the universe backward through a tape of activations?

`job_ai` is my attempt at that answer.

---

## Design principles

**1. Forward-only by default**

* Core training loop is **evolutionary**: population-based search, no stored activations.
* Backprop is not banned; it’s just not the star of the show.
* The “learning step” is: *mutate, evaluate, select* — not “differentiate a giant graph.”

**2. CPU as a first-class citizen**

* Hand-tuned AVX2/tiling GEMM for dense ops.
* FMM / Barnes-Hut style structures for attention and N-body-like workloads.
* Shared memory IPC tuned down to tens of nanoseconds per roundtrip.
* Threading via `job_threads`: work-stealing, high-throughput, designed to saturate cores.

**3. Sparse intelligence, not monolithic blobs**

* Mixture-of-Experts (MoE) where experts are **mapped to roles**:

  * “Crust” experts: IO, sanitization, lightweight transforms
  * “Mantle” experts: physics, continuous dynamics
  * “Core” experts: planning, higher-level structure
* Router can be learned later; initially it can be state-machine / hashed / rule-based.

**4. Physics & geometry as inspiration, not decoration**

* Tokens treated like particles.
* Attention treated like an N-body problem (influence, distance, scaling).
* Evolution treated like geodesic search in a weird, human-in-the-loop manifold.

---

## Core ideas (the short version)

### 1. Evolution instead of backprop (ESCoach)

Instead of:

* Build huge graph
* Store all activations
* Backpropagate gradients

We do:

* Maintain a **population** of candidate networks (`Genome`)
* Perturb weights with noise
* Evaluate each candidate in parallel using `job_threads`
* Select the top performers, update mean & step size (ES-style)
* Repeat


No global autograd tape. Just forward passes, scores, and selection.

---

### 2. Tokens as particles, attention as N-body

Naive attention is O(N²): every token looks at every other token.

In `job_ai`:

* **Tokens** -> particles
* **Q/K/V** -> position / mass / charge analogues
* **Attention** -> influence field

We borrow from FMM / Barnes–Hut style:

* Cluster distant tokens into trees
* Approximate their joint influence
* Push complexity toward **O(N)** instead of O(N²)

This is the route to:

* **Long-context attention on CPU**
* Context windows that don’t immediately explode your RAM .... Hopefully :P 
* A more “physical” intuition for relevance: influence decays with distance (in some learned metric space)

---

### 3. Spatial MoE (Mixture of Experts with geography)

Standard MoE:

* Many experts, a gating network decides which ones to use.

Here:

* We treat the system like a **geology**:

  * **Crust**: near-IO, sanitization, protocol handling
  * **Mantle**: dynamics, control, physical-ish reasoning
  * **Core**: long-term planning, global context

Routing can start simple:

```text
if (input.source == UART)       → Crust experts
if (input.velocity > threshold) → Mantle experts
if (input.planning_depth > k)   → Core experts
```

Later, those can become learned gates — but only if they earn their complexity.

---

### 4. The Portal Evaluator: fitness as “worldline alignment”

When we evaluate a model, we don’t just say “loss = MSE.”
We treat it like: *how close is this system’s trajectory to the intended human trajectory?*

It:

* Runs a `Runner` on each sample.
* Measures squared deviation, with:

  * **NaN/Inf detection** as “singularity” (automatic catastrophic penalty).
* Returns a scalar fitness in (0, 1], where `1.0` ≈ perfect alignment.

It’s “just MSE,” but philosophically:
it’s our little scalar version of “how close the model’s path is to the one we wanted.”

---

## Architecture (high level)

For more information see the section titled "Name spacing" 

Rough layout:

* `job_ai/`
  * `genome.*`  – genome's network topology + weights, etc
  * `runner.*`  – inference engine (builds layers, runs forward)
  * `layers/`   – Dense, Attention, MLP, activations
  * `coach/`    – `ESCoach`, `GeniticCoach`, training loops
  * `adapters/` – Swappable runtime adapter's example FlashAttention, Barnes–Hut, FMM, Dense, Flash, LowRank
  * `learn/`    - xor, cart-pole, portal, etc
  * `moe/`      - Mixure of experts 
  * `router/`   - Routes - Hash, TopK, Spatial(Geology), State

---

## Current state (honest version)

What it **can** do right now:

* Run neural-ish architectures on CPU with:

  * Dense layers (optimized GEMM)
  * FlashAttention variants
  * FMM / Barnes–Hut attention experiments
  * SparseMoE with simple routing
* Evolve networks (like XOR) via ES at very high throughput
* Beat stock PyTorch in some controlled, apples-to-apples microbenchmarks
  (e.g., evolutionary XOR runs **30 times faster** on the same hardware, same task)

What it **does not** do (yet):

* Train giant LLMs
* Replace your day job as an ML engineer
* Magically know what you meant without careful datasets & evaluators

This is a **builder’s engine**:
a lower-level substrate for weird experiments in:

* CPU-only AI
* Evolutionary search
* Sparse, physically-inspired architectures
* Fast, minimal-latency OS-level “thinking modules”

---

## Who is this for?

* A person that looks at 24 GB VRAM requirements and say “nah.”
* You might like C++ templates, AVX intrinsics, and thread pools a little too much.
* You like the idea of:
  - Having a builder networks that are CPU driven, and not GPU/TPU driven.
  - fuck around and find out.

  > Biology runs forward.
  > 
  > Physics runs forward.
  >
  > Maybe some of our learning loops can, too.

---

## Philosophy in one paragraph

I want an AI stack that lives closer to an OS kernel than to a Python script:

* **Think in-place** with millisecond-to-microsecond latency.
* **Learn** (in the evolutionary sense) without needing an activation time machine.
* Treat **tokens as particles**, **experts as geology**, and **fitness as geometry**.
* And it should absolutely, stubbornly, joyfully refuse to pay any tax it doesn’t have to.

A manifold of future time:

> We never go back, we just keep moving forward and see what new structures emerge.

![Minkowski space time](/docs/img/space_time.jpeg "Time")

---

## Status: active experiment

This part of the repository is:

* A lab notebook
* An engine room
* A pile of dragons, some slain, some mid-roar

If you’re reading this, you’re already downstream of a fictional manifesto that accidentally turned into C++.

[Welcome to the Portal](/docs/odd_portal.md)


## Name spacing 
Rules: Three deep before... I go crazy. 
```
|-------------------|---------------|----------------------------------------------------------|---------------------------------------|-------------------------------|
|   namespace       |  desc         |        technical terms (what actually lives here)        |          mapping / notes              |      subsystem                |
|-------------------|---------------|----------------------------------------------------------|---------------------------------------|-------------------------------|
|                   |               |                                                          |                                       |                               |
|                   |               |                                                          |                                       |                               |
|   adapters::      |  algorithms   |      FMM, Barnes–Hut, N-body, Velocity Verlet,           |    bridges core kernels <-> models    |         TRUE                  |
|                   |               |    / bridges treecodes, low-rank attention,              |                                       |                               |
|                   |               |         graph adapters, kernelized attention             |                                       |                               |
|-------------------|---------------|----------------------------------------------------------|---------------------------------------|-------------------------------|
|                   |               |                                                          |                                       |                               |
|                   |               |                                                          |  Question is this in job_core already |                               |
|     base::        |  utils/core   |            small math constants,                         |    ML semantics here;                 |         FALSE                 |
|                   |               |       type traits, RAII helpers, error codes             |    ultra-generic utilities only       |                               |
|                   |               |                                                          |                                       |                               |
|-------------------|---------------|----------------------------------------------------------|---------------------------------------|-------------------------------|
|                   |               |                                                          |                                       |                               |
|                   |               |                     Trainer                              |                                       |                               |
|    coach::        |  motivation   |        ES / CMA-ES, genetic algorithms, population       |    training loops, schedulers,        |         TRUE                  |
|                   |               |          sampling, fitness functions, curriculum         |    “optimizers” in the broad sense    |                               |
|                   |               |                                                          |                                       |                               |
|                   |               |                                                          |                                       |                               |
|-------------------|---------------|----------------------------------------------------------|---------------------------------------|-------------------------------|
|                   |               |                                                          |                                       |                               |
|                   |               |                                                          |                                       |                               |
|     cords::       |  places       |        tensor shapes, strides, layouts, views,           |    all the tensor_* stuff,            |         TRUE                  |
|                   |               |          broadcasting, slicing, memory mapping           |    memory ownership & layout          |                               |
|                   |               |                                                          |                                       |                               |
|-------------------|---------------|----------------------------------------------------------|---------------------------------------|-------------------------------|
|                   |               |                                                          |                                       |                               |
|                   |               |                                                          |                                       |                               |
|     comp::        |  math         |       BLAS-y kernels (GEMM, batched GEMM),               |    hot math: SIMD kernels,            |         FALSE                 |
|                   |               |        activation kernels, reductions,                   |    reductions, norms, layernorm,      |                               |
|                   |               |        normalizations (LayerNorm/RMSNorm),               |    softmax, small convs, etc.         |                               |
|                   |               |        dot products, softmax, small conv windows         |                                       |                               |
|                   |               |                                                          |                                       |                               |
|-------------------|---------------|----------------------------------------------------------|---------------------------------------|-------------------------------|
|                   |               |                                                          |                                       |                               |
|                   |               |                                                          |                                       |                               |                                                                                                                                                                
|      evo::        |  directions   |         mutation operators, crossover, selection         |    runtime helpers for evolution:     |         FALSE                 |
|                   |               |          speciation / NEAT-like structures,              |    how to generate / combine genomes  |                               |
|                   |               |          genome encodings, schedule policies             |    but not the trainers themselves    |                               |
|                   |               |                                                          |                                       |                               |
|-------------------|---------------|----------------------------------------------------------|---------------------------------------|-------------------------------|
|                   |               |                                                          |                                       |                               |
|                   |               |                                                          |                                       |                               |
|      infer::      |  inference    |      runtime drivers: forward pass orchestration,        |    “serving” side: runners,           |         FALSE                 |
|                   |               |      sampling (top-k, nucleus), beam search,             |    batching, token loops, sched glue  |                               |
|                   |               |        streaming, KV-cache plumbling, runners            |                                       |                               |
|                   |               |                                                          |                                       |                               |
|-------------------|---------------|----------------------------------------------------------|---------------------------------------|-------------------------------|
|                   |               |                                                          |                                       |                               |
|                   |               |                                                          |                                       |                               |
|     cache::       |  key/values   |          KV-cache, associative buffers, eviction         |    attention KV cache,                |          FALSE                |
|                   |               |             policies (LRU/LFU/custom),                   |    other (key → state) maps,          |                               |
|                   |               |           segmenting by head/layer                       |    on-CPU cache layout                |                               |
|                   |               |                                                          |                                       |                               |
|-------------------|---------------|----------------------------------------------------------|---------------------------------------|-------------------------------|
|                   |               |                                                          |                                       |                               |
|                   |               |                                                          |                                       |                               |
|     layers::      |    layers     |           Linear, MLP, Attention, Embedding,             |     higher-level building blocks      |         TRUE                  |
|                   |               |          LayerNorm/RMSNorm, Conv (if any),               |                                       |                               |
|                   |               |          positional encodings, residual blocks           |                                       |                               |
|                   |               |                                                          |                                       |                               |
|                   |               |                                                          |                                       |                               |
|-------------------|---------------|----------------------------------------------------------|---------------------------------------|-------------------------------|                                                                       
|                   |               |                                                          |                                       |                               |
|                   |               |                                                          |                                       |                               |
|      moe::        |   experts     |       experts, SparseMoE                                 |     sparse MoE: Expert interface,     |         TRUE                  |
|                   |               |                                                          |     router, dispatcher, expert        |                               |
|                   |               |                                                          |             configs                   |                               |
|-------------------|---------------|----------------------------------------------------------|-----------------------------------------------------------------------||                                                                       
|                   |               |                                                          |                                       |                               |
|                   |               |                                                          |                                       |                               |
|      router::     |   routers     |       routers types, Spatial , hash, top-k,              |                                       |         TRUE                  |
|                   |               |          state, irouter and classes based on irouter     |                                       |                               |
|                   |               |          config(RouterConfig) with presets               |                                       |                               |
|-------------------|---------------|----------------------------------------------------------|-----------------------------------------------------------------------|                                                                       
|                   |               |                                                          |                                       |                               |
|                   |               |                                                          |                                       |                               |
|      learn::      |   evaluator   |       xor, suduko, cart-pole, more incoming              |                                       |         TRUE                  |
|                   |               |                                                          |                                       |                               |
|                   |               |                                                          |                                       |                               |
|-------------------|---------------|----------------------------------------------------------|-----------------------------------------------------------------------|
```
