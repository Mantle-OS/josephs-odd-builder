# Job AI Router

Routing layer for MoE in Job.

`job::ai::router` turns “batch rows” into a routing plan:
- which expert(s) each row goes to
- with what weight
- and what adapter type to run for that expert

The router stays planning only. Execution happens in the MoE layer.

## Output: RouterPlan / RouterToken

Routing output is a flat list of tokens.

Each `RouterToken` is one assignment:
- `row`     : batch row index
- `expert`  : expert index
- `weight`  : gate weight (prob-like)
- `adapter` : adapter type selected from the expert config

`RouterPlan` is just metadata + a non-owning pointer to the token buffer:
- `batchSize`
- `numExperts`
- `tokens` + `tokenCount`

The token buffer is typically carved out of an inference workspace.

## Config: RouterConfig / RouterExpertConfig

`RouterConfig` holds:
- router type (`RouterType`)
- load-balance strategy enum (`LoadBalanceStrategy`) (carried in config)
- `experts[]` : per-expert config
- `topK` : number of assignments per row
- extra knobs (hash temperature, spatial radius, deterministic flag)

`RouterExpertConfig` holds:
- `id`
- `adapter` (`adapters::AdapterType`)

Adapter type is attached into each `RouterToken` during planning so the MoE layer can run heterogeneous experts.

## Router types (current behavior)

### TopK
Learned gating mode.

Input is a dense logit matrix (batch x experts) produced by the MoE layer.
Routing selects the top-K experts per row and normalizes weights across those K.

### Hash
Deterministic assignment mode.

Produces K assignments per row with uniform weights.
Expert indices are currently assigned in a simple repeating pattern.

### Spatial
Deterministic assignment mode with “spatial” label.

Produces K assignments per row with uniform weights.
Expert indices are currently assigned in a simple repeating pattern.

### State
Deterministic assignment mode with “state” label.

Produces K assignments per row with uniform weights.
Expert indices are currently derived from the row index (rotating window across experts).

