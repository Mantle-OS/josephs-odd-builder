# Job AI Coach

The coach for `job::ai` evolution.

job:
- `job::ai::evo` = genomes + mutation + population storage
- `job::ai::learn` = “score this genome” environments
- `job::threads` = pool + `parallel_for` for population eval
- `job::ai::infer` = runner used by dataset-style evaluators

## Types

### CoachType
- `Genetic` (placeholder)
- `ES` (implemented)
- `CMA_ES` (placeholder)

### OptimizationMode
- `Maximize` (higher fitness wins)
- `Minimize` (lower score wins)

## ICoach

Common coach shape:
- population sizing
- mutation rate knob
- optimization mode
- `coach(parent)` returns a new genome (current best)
- generation counter + current best fitness
- `learner()` returns the GUI/thread-owner learner instance (for interactive runs)


## ESConfig

Config for ES:
- `envConfig` (`job::ai::learn::LearnConfig`)
- `populationSize`
- `coachSeed` (stored; current ES path seeds per-gen/idx)
- `sigma` (mutation strength)
- `decay` (sigma anneal)

Presets (`CoachPresets`):
- `kStandard` (XOR-ish defaults)
- `kBard` (high volatility / smaller pop)

## ESCoach (`es_coach.h`)

(1, λ) evolution strategy.

Loop:
- population entry starts as a clone of `parent`
- each clone gets Gaussian perturbation (sigma)
- each candidate is scored
- best candidate becomes the returned genome

Parallelism:
- population evaluation runs under `job::threads::parallel_for(*pool, 0..popSize)`
- each worker thread keeps its own `job::ai::learn::ILearn` instance (thread-id registry)
- a separate learner instance is kept for “GUI / owner thread” access

Selection:
- best is picked using `OptimizationMode` (max or min)
- best fitness is stored as `currentBestFitness()`

Anneal:
- if `decay` is in (0,1), sigma is multiplied by decay each generation

## Dataset-style evaluators

### TrainingSample (`fitness_evaluator.h`)
- `input`  = context vector
- `target` = expected output vector

### IFitnessEvaluator
- `evaluate(runner, dataset)` returns a single float score for a genome/run

