# Contexts

The JOB GGML context subsystem wraps GGML context initialization, ownership, borrowed context access, and computation graph creation.

Contexts provide the memory arena in which GGML tensor metadata and graphs are created. `JobGgmlInitParams` describes how that arena should be initialized, while `JobGgmlContext` owns the resulting native context. `JobGgmlContextView` provides non-owning access when another GGML object controls the context lifetime, and `JobGgmlCGraph` wraps computation graphs created within a context.

## JobGgmlInitParams

`JobGgmlInitParams` is the JOB representation of `ggml_init_params`. It stores the context memory size, optional external memory buffer, and GGML `no_alloc` state used when creating a context.

The memory buffer is borrowed from the caller and is never owned by `JobGgmlInitParams`. The object may be constructed from individual initialization values or from an existing native `ggml_init_params`, and the current JOB-side state can be converted back to the native representation through `initParams()`.

Metadata-oriented factory helpers are provided for contexts that primarily store tensor and graph metadata. A fixed memory size may be supplied directly, or `createMetadataFor()` and `createUniqMetadataFor()` can estimate the required context size from the expected number of tensor objects, graph size, gradient requirements, and optional additional padding.

Context-size estimation is deterministic for metadata-only contexts. `estCtxCost()` combines the native GGML tensor-object overhead for the requested tensor count with the graph overhead reported by `JobGgmlCGraph::overheadCustom()`. Optional padding may be added explicitly by the caller when additional headroom is required.

`setInitParams()` replaces the complete initialization state from a native `ggml_init_params`, while `resetInitParams()` restores the default configuration.


## JobGgmlContext

`JobGgmlContext` is the owning JOB wrapper for a native GGML context. A context provides the memory arena used to create tensor metadata, computation graphs, and related GGML objects.

A context may be created from `JobGgmlInitParams`, directly from a native `ggml_init_params`, or by taking ownership of an existing `ggml_context_ptr`. Context creation fails if GGML cannot initialize the requested context or if an invalid native context is supplied.

The wrapper exposes the current context memory state, including total context size, used memory, maximum tensor size, the underlying memory buffer, and the current `no_alloc` setting. Additional raw storage may be allocated from the context through `newBuffer()`. :contentReference[oaicite:1]{index=1}

`JobGgmlContext` is also the primary tensor factory for context-owned GGML tensors. Tensors may be created with one through four dimensions or through the general dimensional interface, duplicated from existing tensors, or created as views of another tensor. The context also provides tensor iteration and lookup by name. :contentReference[oaicite:2]{index=2} :contentReference[oaicite:3]{index=3}

Computation graphs are created through the context as `JobGgmlCGraph` wrappers. Callers may create a graph using the GGML default configuration, create a custom graph with a selected capacity and gradient support, or duplicate an existing graph into the context. :contentReference[oaicite:4]{index=4}

For contexts that store both metadata and tensor payload data internally, `createHostContext()` and `createUniqHostContext()` combine the deterministic metadata estimate provided by `JobGgmlInitParams::estCtxCost()` with the required payload size and optional padding before constructing an allocating context.

`JobGgmlContext` can also retain arbitrary custom-operation payloads through `createPayload()`. These payloads are owned by the context so callback or custom-operation state remains alive for the lifetime of the graph objects that depend on it. Calling `reset()` resets the native GGML context and releases all retained custom payloads. :contentReference[oaicite:5]{index=5}

## JobGgmlContextView

`JobGgmlContextView` provides non-owning access to a native `ggml_context` whose lifetime is controlled by another GGML object.

The immediate use case is `JobGgmlBackendGraphCopy`, whose native graph-copy aggregate owns both an allocated and an unallocated context and releases them together with the rest of the aggregate. Wrapping either context in `JobGgmlContext` would create an incorrect second owner.

`JobGgmlContextView` therefore borrows the supplied native context and never releases it. The native owner must outlive the view and every tensor wrapper obtained through it.

The view exposes the same core context-inspection vocabulary needed by callers, including used memory, total context size, maximum tensor size, the underlying memory buffer, and the current `no_alloc` state.

Existing tensors may be inspected through iteration and lookup helpers. Callers can obtain the first tensor, advance to the next tensor, or find a tensor by name without taking ownership of the underlying native context.

Unlike `JobGgmlContext`, the view does not expose context reset, memory allocation, tensor creation, graph creation, or custom-payload ownership. These operations are intentionally omitted because they belong to the native context owner.

## JobGgmlCGraph

`JobGgmlCGraph` is the JOB wrapper for a native GGML computation graph. The underlying `ggml_cgraph` is borrowed from the owning GGML context and is never released independently by the graph wrapper.

The graph exposes its configured size and current node count, and individual graph nodes may be inspected by index or collected as `JobGgmlTensor` wrappers. Tensors may also be looked up by name, while gradient and gradient-accumulator tensors can be queried for graph nodes when gradient information is available.

Forward graphs may be constructed by selecting from a collection of tensors or by expanding from a result tensor. Backward graph construction is also supported through an owning `JobGgmlContext`, with optional gradient accumulators supplied by the caller.

Nodes may be added explicitly to the graph, while `reset()` and `clear()` expose the corresponding graph lifecycle operations without affecting ownership of the containing context.

For diagnostics, a graph may be printed through GGML or exported as a Graphviz DOT file. A forward graph may optionally be supplied when generating the DOT representation.

The native `ggml_cgraph` remains available through `graph()` for lower-level interoperability. Static `overhead()` and `overheadCustom()` helpers expose GGML's graph metadata cost and are used by `JobGgmlInitParams` when calculating deterministic context-memory requirements.










