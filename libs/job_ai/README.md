## A manifold of future time 

Joseph drinks a bunch of coffee and then speak's.....

A place where past time has not meaning as it is never traveled. We do propagate back there..... 

I want AI, but I reject the "GPU Tax" (High VRAM, CUDA lock-in) and the "Algorithm Tax" Backpropagation, which requires storing the entire activation graph in memory (no constrants at all lol).

If we are going to do this ....shit ...  we have to look at how biology & composition & physics do it.

* Biology doesn't do Backpropagation. Neurons don't send error signals backward up the axon. 
* Physics doesn't do Backpropagation time flows one way. Try and observe that.

Lets say a number like ```5, 20, 50, 100.```

20 distributed nodes and a massive Work Stealing Scheduler. 

I don't need PyTorch nor do I want it. 

I need population based training and sparse intelligence.

1. The Death of Backprop: "Forward-Only" Evolution

Since job already has a highly parallel simulator (job_threads) and a cluster (job_net), the most efficient way to train is neuro evolution(Genetic Algorithms) ... I think lol 


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


NEED MORE 
**Cons**
*  
* 
* 

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


## GNC

```text
├── base
│   ├── activation          // The vanishing problem (ReLU, Tanh, Swish)
│   ├── genome              // CATGACGTC ... The God Thread
│   └── tensor_view         // Non-owning view of memory (mmap friendly).
├── coach
│   ├── es_trainer          // (CMA-ES)
│   ├── genetic_trainer     // The sticky fingers(the thief) .... :P 
│   └── icoach              // the purest virtual form of motivational
├── evo
│   ├── crossover           // look there are more of you.... 
│   ├── mutator             // Its loud here in the radiation of gaussian doing flips 
│   ├── population          // what a bunch or weirdo mutants
│   └── species             // The genus type of kind category... NEAT !
├── infer
│   └── runner              // run forest run right to the engine room
├── machine
│   ├── att
│   │   ├── fmm_adapter     // Math dragons
│   │   ├── bh_adapter      // Joshua sits in his Hut thinking of basking in the direct sum light  
│   │   └── kv_cache        // The what just happend. 
│   ├── dense               // The perceptron of moving forward
│   ├── ilayer              // a pure form of inference of interface's to layers
│   └── sparse_moe          // The geological router
```

## The math fun


