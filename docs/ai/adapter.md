# Job AI Adapter

Adapter layer for `job::ai`.

Adapters are the “backend choice” for a few ops (mostly attention and MoE):
- pick an implementation strategy
- keep any scratch state
- run the forward pass using `job::ai::comp` kernels

job:
- `job::ai::adapter` = adapter objects + factory
- `job::ai::comp` = kernels used by adapters
- `job::ai::cords` = views used to pass buffers
- `job::threads` = ThreadPool used for parallel paths

## AdapterType

Enum describing the adapter flavor:
- `Dense`
- `FlashAttention`
- `SparseMoE`
- `None`

Some enums exist as placeholders for future engines.

## IAdapter

Common shape for adapters:
- has a type
- can be reset between runs
- takes a scratch workspace view
- runs a forward pass given input/output views and a config struct

Adapters are used by layers to keep the layer code small while still allowing swap the engine.

## Attention adapters

Attention work is routed through an adapter so the layer doesn’t hardcode a single kernel path.

Current attention adapter lanes:
- dense attention adapter (plain forward path)
- flash-style attention adapter (blocked forward kernel)

Both call into `job::ai::comp` for the heavy math and optionally use the pool for row-block parallelism.

## MoE adapters

MoE uses adapter types per expert (carried in router tokens).

Current setup:
- adapter type is stored in expert config
- router copies that adapter type into each routing token
- MoE execution picks the adapter for each expert block

Plumbing for heterogenous experts.

## AdapterFactory

Creates adapter instances from `AdapterType`.

This is used by:
- layer factory when building Attention layers
- SparseMoE when constructing experts and their engine choice
