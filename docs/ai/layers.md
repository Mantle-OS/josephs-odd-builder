# Job AI Layers

Layer stack for `job::ai`.

Things that run:
- layer configs + enums
- base layer interfaces
- a few concrete layers (Dense / Attention / SparseMoE)
- a small factory that builds layers from `evo::LayerGene`

job:
- `job::ai::layers` = layer objects + configs
- `job::ai::comp` = math kernels used by layers (sgemm, attention forward, activations)
- `job::ai::infer` = workspace + runner that calls `forward(...)`
- `job::ai::router` = routing plan used by MoE
- `job::threads` = ThreadPool used for parallel passes


## Types / config
### LayerType
Enum for the layer kind:
- Input / Dense / SparseMoE / Attention / Embedding / (LayerNorm / RMSNorm / Residual) / LinearLoRA / Output

Only a subset has real implementations in this snapshot (Dense, Attention, SparseMoE).

### LayerMode
Training vs Inference switch carried in config.

### LayerFlags
Bit flags used by some layers:
- causal
- has router
- has bias

### LayerConfig
Small “one struct” config used by `AbstractLayer`:
- name (short fixed char array)
- inputs / outputs
- attention knobs: maxSequence, numHeads
- MoE knobs: numExperts, topK
- adapterType (engine choice)
- activation type
- flags (causal/router/bias)


## Base interfaces

### ILayer
The small metadata + parameters interface:
- layer type + name
- mode setter
- flat parameter view (non-owning)
- parameter count
- alpha knob (used by some activation paths)

### AbstractLayer
The base used by the runner:
- forward pass over `cords::ViewR` buffers
- load weights from a flat float block
- scratch sizing (workspace planning)
- output shape (extent in/out)

base type returned by `LayerFactory`.


## Parameter grouping (inspection)

### IParamGroup
Optional interface that exposes named parameter groups.

Used for things like:
- splitting weights vs bias
- surfacing gate/router weights (MoE)
- surfacing sub-layer params inside a composite layer

### ParamGroupType
Buckets:
- Weights
- Bias
- GateWeights
- ExpertWeights
- RouterState
- Other

slices of the flat param blob with labels.

## Concrete layers

### DenseLayer
Standard linear projection:
- input [rows, inputs] to output [rows, outputs]
- optional bias add
- optional activation
- switches to threaded loops when the matrix is big enough

This layer owns its weight/bias pointers (into a flat storage block loaded via `loadWeights`).

### AttentionLayer
Attention block built from projections + an adapter:
- projects X into Q/K/V (matrix multiply)
- optional bias add to Q/K/V/O paths
- runs an attention adapter (engine selected by config)
- writes back into the output buffer

Also exposes parameter groups for:
- Wq/Wk/Wv/Wo
- optional bq/bk/bv/bo

### SparseMoE
Mixture of Experts layer:
- gate weights produce logits for routing (TopK mode)
- router produces a token list `(row, expert, weight, adapter)`
- input rows are packed per-expert into contiguous blocks (gather)
- each expert runs on its packed block (forward pass)
- results are scattered back into output with per-token weight (weighted sum)

Workspace usage is explicit:
- routing logits + routing tokens live at the front
- packed expert input/output buffers follow
- scratch is carved per expert

Parameter groups include:
- gate weights as `GateWeights`
- optional groups forwarded from experts that implement `IParamGroup`

## LinearLoRA
### Alpha not done
Low-rank adapter layer (separate from `AbstractLayer` in this snapshot):
- holds low-rank A and B matrices (rank r)
- applies the LoRA update using GEMM kernels
- can reference a shared “frozen weights” block (external storage)

currently its own ILayer ...

## Factory

### LayerFactory
Builds an `AbstractLayer` from `evo::LayerGene`.
Implemented paths in this snapshot:
- Dense
- Attention
- SparseMoE (also builds a set of Dense experts and sets router type)
