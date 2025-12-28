# Job AI Evo

Population utilities for `job::ai`.

weights evolve layer:
- a `Genome` type (architecture + weight blob + binary save/load)
- `Mutator` (noise on weights)
- `Crossover` (mix weights between two parents)
- `Population` (elitism + simple selection + offspring loop)
- a stub `SpeciationEngine` + `Species` structs (not active)

job:
- `job::ai::layers` + `job::ai::comp` provide enums used by `LayerGene`
- `job::crypto` provides RNG (`job_random.h`) used by population/mutation


## Genome

### LayerGene
One layer description used inside a genome:
- `type` (layer enum)
- `activation` (activation enum)
- `inputs`, `outputs`
- `weightOffset`, `weightCount`
- `biasOffset`, `biasCount`
- `auxiliaryData` (misc knob storage)

Size is fixed (static assert) and padding is explicit (zeroed).

### GenomeHeader
Header fields used for bookkeeping:
- magic/uuid/parentId
- generation
- fitness
- layerCount
- weight blob size

### Genome
The actual payload:
- `header`
- `tested` flag
- `architecture` (vector of `LayerGene`)
- `weights` (flat float blob)

Binary save/load exists:
- file magic `"GENO"`
- fields are written explicitly (not `sizeof(struct)` dumps)

## Mutation

### Mutator
Weight noise engine.

Current behavior:
- perturb(genome) walks the weight blob
- each weight is mutated with probability weightMutationProb
- mutation is additive Gaussian noise with sigma `weightSigma`
- optional `seed(...)` for deterministic runs


### Crossover
Combines two parent genomes into a child.

Current behavior is weight-blob mixing:
- parents must have the same weight count (guarded)
- child starts as a clone, then weights are mixed according to `CrossoverType`

Implemented modes:
- None
- OnePoint / TwoPoint
- Uniform
- Arithmetic

### Population
Small generation loop around `Genome` + fitness array.

Current flow:
- fitness values are stored alongside the genomes
- genomes are sorted by fitness (higher is better)
- elites are copied into the next generation (`eliteCount` capped to population size)
- the mating pool is the top half of the population
- parents are selected uniformly from that pool
- offspring is produced via `Crossover` then `Mutator`
- next generation replaces current population, and fitness is reset

PopulationConfig holds population size, elite count, and the crossover/mutator configs.
