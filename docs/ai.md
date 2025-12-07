## A manifold of future time 

Joseph drinks a bunch of coffee and then speak's.....

A place where past ```time``` has not meaning as it is never traveled. We do not propagate back there..... 

I want AI, but I reject the "GPU Tax" (High VRAM, CUDA lock-in) and the "Algorithm Tax" Backpropagation(unless needed), 
which requires storing the entire activation graph in memory (no constrants at all ...lol).

If I am going to do this ....shit ...  I have to look at how biology & composition & physics do it.

* Biology doesn't do Backpropagation. Neurons don't send error signals backward up the axon. 
* Physics doesn't do Backpropagation time flows one way. Try and observe that.

Lets say a number like ```5, 20, 50, 100.```

20 distributed nodes and a massive Work Stealing Scheduler. 

I don't need PyTorch nor do I want it. 

I need population based training and sparse intelligence.

1. The Hybrid of Biology in Backprop: "Forward-Only"  Evolution unless abousoulty needed

Since job already has a highly parallel simulator (job_threads) and a cluster (job_net), Ipc(job_io), etc etc   

The most "efficient" way to train is neuro evolution(Genetic Algorithms) ... I think lol 


The Concept: Instead of calculating the gradient $\nabla$ (which is expensive), I generate 100 "mutants".
  - Create 100 job_threads tasks. Each has a slightly different perturbed random noise.
  - satellites, trains(lol), the physics sim for 5 seconds.
  - crashed ? minimized the error ?
  - clone the top 50%
  - rinse and repeat

**Pros**
* No Memory Graph no stored activations. 
* RAM usage is minimal.
* Every thread runs independently.
* No synchronization
* Topology Search" (NEAT - NeuroEvolution of Augmenting Topologies)
* I dont have to buy hardware that I dont need for my builder network (Mantle Os)


---

Distant particles to calculate gravity efficiently ($O(N)$ instead of $O(N^2)$). Particles calculating influence on each other ($O(N^2)$

> Linear attention -> FMM / Barnes-Hut etc -> "Semantic Relevance."

* Tokens  = Particles.
* Q/K/V   = Position/Mass.
* The Job = fmm, bh, rk4, carrier pidgons to calculate the context vector without building the massive matrix.

This could allow us to run massive context windows on CPU, because we are using a tree approximation instead of a matrix multiplication.

3. Moe from the simpsons
MoE is still heavy. Let's build a Spatial MoE.
* we have a "Geology." Let's map experts to the geology.
  - Expert A (The Crust): Handles basic IO / UART sanitization.
  - Expert B (The Mantle): Handles Physics / Dynamics.
  - Expert C (The Core): Handles long-term planning / Strategy.
  
The Router: Instead of a learned gating network(which needs backprop), we could use a hashed router or a "state machine router"
* Input comes from UART? -> Route to Expert A.
* Input velocity > threshold? -> Route to Expert B.


Rules: three deep before I go crazy. 


## Name spacing 
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

```


                                      
### base namespace
```c++
job::ai::
```


### namespace types
There are two types of namespace's 
1. helpers / utils runtime one offs
2. modules 
 
### helpers
| namespace  |      desc          |            technial terms             |       mapping / notes       |   module   |
|------------|--------------------|---------------------------------------|-----------------------------|------------|
| base::     |    utils           | core                                  |                             |    FALSE   |
| comp::     |    math            | kernels & math                        |                             |    FALSE   |
| evo::      |    directions      | directions                            |    runtime helper           |    FALSE   |
| infer::    |    inference       | runtime helpers                       |                             |    FALSE   |
| kv::       |    key vales       | cache and other bits)                 |                             |    FALSE   |


### subsystems

Each module has the following mandatory or optional area's

|module interface| modules have interfaces to what there core function are |mandatory  |
|----------------|---------------------------------------------------------|-----------|
|types           |  all modules have a enum of types                       |mandatory  |
|configs         |  what makes this uniq                                   |optional   |
|presets         |  types plus configs pus interface  presets              |optional   |

### subsystem's

| namespace  |      desc          |            technial terms             |       mapping / notes       |   module   |
|------------|--------------------|---------------------------------------|-----------------------------|------------|
| adapters:: |    algorithms      | used in directions (FMM, BH, VV etc)  |  [virtual, types, configs]  |    TRUE    |
| coach::    |    motivation      | trainer's                             |  [virtual, types, configs]  |    TRUE    |
| cords::    |    places          | tensor's                              |  [virtual, types, configs]  |    TRUE    |
| layers::   |    layers          | hidden, input, output layers etc      |  [virtual, types, configs]  |    TRUE    |
| moe::      |    experts         | mixure of experts  & router's         |  [virtual, types, configs]  |    TRUE    |


### all
| namespace  |      desc          |            technial terms             |       mapping / notes       |   subsystem |
|------------|--------------------|---------------------------------------|-----------------------------|-------------|
| adapters:: |    algorithms      | used in directions (FMM, BH, VV etc)  |  [virtual, types, configs]  |    TRUE     |
| base::     |    utils           | core                                  |                             |    FALSE    |
| coach::    |    motivation      | trainer's                             |  [virtual, types, configs]  |    TRUE     |
| cords::    |    places          | tensor's                              |  [virtual, types, configs]  |    TRUE     |
| comp::     |    math            | kernels & math                        |                             |    FALSE    |
| evo::      |    directions      | directions                            |    runtime helper           |    FALSE    |
| infer::    |    inference       | runtime helpers                       |                             |    FALSE    |
| kv::       |    key vales       | cache and other bits)                 |                             |    FALSE    |
| layers::   |    layers          | hidden, input, output layers etc      | [virtual, types, configs]   |    TRUE     |
| moe::      |    experts         | mixure of experts  & router's         | [virtual, types, configs]   |    TRUE     |

