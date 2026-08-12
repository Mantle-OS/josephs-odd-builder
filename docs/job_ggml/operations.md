# Job GGML Operations

The operations subsystem provides the JOB GGML interface for constructing tensor operations and computation expressions.

`JobGgmlTensorOp` builds on `JobGgmlTensor` by associating a tensor with the `JobGgmlContext` required to create new GGML operation tensors. Operations return new `JobGgmlTensorOp` objects, allowing tensor expressions to be composed while remaining within the JOB C++ interface.

The operation API closely follows the functionality exposed by GGML while accepting JOB tensor wrappers and JOB enum types where appropriate.

---


## JobGgmlTensorOpGraph

`JobGgmlTensorOpGraph` represents the graph-building side of a tensor operation expression. It derives from `JobGgmlTensorOp`, preserving the complete operation-construction interface while adding the ability to materialize the current expression as a `JobGgmlCGraph`.

The wrapper retains the same borrowed `JobGgmlContext` used by the operation chain. `buildGraph()` creates a new computation graph from that context and expands the current tensor expression forward into the graph.

An existing `JobGgmlTensorOp` may be promoted to the graph-building interface through `wrap()`. The resulting `JobGgmlTensorOpGraph` references the same native tensor and context rather than creating a duplicate operation tensor.

This allows an operation chain to remain lightweight while it is being composed and only become a concrete computation graph when graph construction is actually required.



## JobGgmlTensorOp

`JobGgmlTensorOp` is the primary tensor-operation interface in JOB GGML. It derives from `JobGgmlTensor`, preserving the complete tensor inspection API while adding the context required to construct new operation tensors.

The associated `JobGgmlContext` is borrowed and must remain valid while operations are being constructed. Operation arguments are validated before being passed to GGML, and JOB tensor wrappers are converted internally to their native `ggml_tensor` representations.

The operation surface is divided into four broad groups.

### Unary Operations

Unary operations construct a new tensor expression from the current tensor without requiring another primary tensor operand.

This group includes duplication, mathematical functions, reductions, sign and absolute-value operations, activation functions, exponential and rounding operations, GLU-family operations, contiguous conversion, transpose, diagonal construction, and softmax.

Where GGML exposes both allocating and in-place forms, JOB generally provides matching methods such as `sqr()` and `sqrInplace()`, `relu()` and `reluInplace()`, or `softMax()` and `softMaxInplace()`.

Unary operations include common activation functions such as ReLU, ELU, sigmoid, GELU, SiLU, hard-swish, and hard-sigmoid together with GLU-family forms including ReGLU, GEGLU, SwiGLU, and their swapped variants.

### Binary and Multi-Tensor Operations

Binary and multi-tensor operations combine the current tensor with one or more additional tensors.

The group includes element-wise arithmetic, equality counting, repetition and repetition-backward operations, concatenation, split GLU operations, matrix multiplication, outer products, tensor copies, row selection and updates, and related backward operations.

Matrix operations include ordinary matrix multiplication through `mulMat()`, indexed matrix multiplication through `mulMatId()`, and outer-product construction through `outProd()`.

Additional tensor arguments may be supplied as JOB tensor objects or compatible tensor wrappers. They are validated and converted to native tensor pointers internally before the corresponding GGML operation is constructed.
### Parameterized Transformations and Views

Parameterized transformations modify the shape, layout, normalization, scaling, masking, or interpretation of tensor data while constructing new GGML graph nodes.

This group includes accumulation and set operations, normalization families, scaling and bias operations, casting, contiguous reshaping, explicit reshape operations, tensor views, permutation, diagonal masking, and extended softmax operations.

Normalization support includes standard, RMS, group, and L2 normalization together with available in-place and backward variants. Matrix-multiplication precision and operation hints may also be configured through the current operation tensor.

Shape and layout transformations include one- through four-dimensional contiguous conversion, reshaping, and explicit views with caller-selected extents, strides, and offsets. `permute()` provides dimension reordering without requiring direct use of the native GGML interface.

Extended softmax operations support masks, scale and bias parameters, sink tensors, backward construction, and in-place forms where provided by GGML.

### Specialized Operations

The specialized operation group exposes the larger model- and workload-oriented operations provided by GGML.

This includes rotary position embeddings, convolution and image-column transformations, pooling, upscaling and interpolation, padding and rolling, sorting and top-K selection, Flash Attention, state-space-model operations, relative-position operations, recurrent and gated attention operations, triangular solving, loss construction, and optimizer-step operations.

RoPE support includes ordinary, extended, multi-position, in-place, and backward forms. Convolution support spans one-, two-, and three-dimensional operations together with depthwise, transposed, direct, and associated `im2col`/`col2im` transformations.

Attention-related functionality includes Flash Attention configuration and backward construction, relative-position helpers, gated linear attention, RWKV operations, gated delta networks, and related model-specific primitives.

The group also contains utility and model-building operations such as clamping, pooling, interpolation, padding, timestep embeddings, sorting, range construction, cross-entropy loss, and native AdamW and SGD optimizer steps.


#### Custom Operations

`JobGgmlTensorOp` also provides callback-backed custom operations for one, two, three, or arbitrary tensor inputs. JOB uses `std::function` callback types and retains the associated callback payload through `JobGgmlContext`, ensuring that callback state and optional user data remain alive for the lifetime required by the constructed graph.

Static trampoline functions bridge the native GGML callback ABI to temporary `JobGgmlTensor` wrappers before invoking the C++ callback.


