# Job AI Infer

Inference runner + scratch workspace for `job::ai`.

job:
- `job::ai::infer` = runner + workspace
- `job::ai::layers` = actual layer implementations (runner builds these)
- `job::threads` = `ThreadPool` used to run layer forward passes
- `job::ai::cords` = `ViewR` tensor views (input/output)


## Runner (`job::ai::infer::Runner`)

Owns a built network from an `evo::Genome`:
- walks `genome.architecture`
- builds a list of `layers::AbstractLayer` via `layers::LayerFactory`
- keeps a max layer dimension for buffer sizing

Run path:
- uses a single `Workspace` allocation as the backing store
- splits that workspace into:
  - two ping-pong float buffers (layer in/out)
  - one scratch region (passed into layers as a `Workspace` view)
- forwards layer-by-layer, flipping buffers each layer
- returns a `cords::ViewR` that points into the internal workspace memory

Reload path:
- if the new genome is weight-compatible: updates weights in-place
- otherwise rebuilds the layer list from the new architecture

Reset path:
- calls into each layer to clear internal state (cache / running stats / etc)

## Workspace
Aligned float storage used as scratch for inference.

Two modes:
- owning buffer (aligned `std::vector<float>`)
- view buffer (non-owning pointer + byte size)

Used to avoid per-layer heap churn during forward passes.


## WorkspaceArena

Bump allocator on top of a `Workspace`:
- cursor in bytes
- aligned allocations for bytes or floats
- returns raw pointers into the workspace

Used by code that wants “allocate temp blocks sequentially” inside scratch.

## WorkspaceView
Small non-owning view type:
- pointer + byte count
- no-op resize

