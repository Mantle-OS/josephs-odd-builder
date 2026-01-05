# Job AI Comp

Compute kernels for `job::ai`.

math:
- GEMM (float32)
- activations
- MLP glue (gate/value/proj)
- attention forward (CPU)
- a few small support pieces (atomic add, noise table, config structs)

job:
- `job::ai::comp` = kernels
- `job::simd` = backend selection (AVX/NEON)
- `job::threads` = `ThreadPool` + `parallel_for` for the parallel wrappers
- `job::ai::cords` = matrix / tensor views used by some helpers

## Activation

Activation functions live as small “functors” with:
- SIMD path
- scalar path

The activation set in-tree covers the usual suspects:
- Identity
- Sigmoid / Tanh
- ReLU family (plain/leaky/etc)
- ELU
- GELU
- Swish
- Softplus
- Mish

etc

`activation.h` also contains fused paths where activation is applied as part of a larger kernel.


## GEMM
`gemm.h` is float32 matrix multiply plus wrappers.

Shape:
- naive reference multiply
- SIMD tiled kernel path
- parallel wrappers using `job::threads::parallel_for`
- convenience wrappers for `cords::Matrix` and `cords::ViewR`
- strided batched variants (same idea, repeated with offsets)

base for most of the layer math in other folders.


## MLP

`mlp.h`:
- gate projection
- value projection
- activation on the gate side
- elementwise multiply (gate * value)
- projection back to model width

Has a direct path and a parallel path (activation/multiply chunking on the hidden buffer).

## Attention (forward)

`flash_attention.h` is a CPU forward attention kernel:
- blocked by row/col tiles
- stable softmax per row (tracks local max + running sum)
- writes output tile as it goes

Also includes a parallel wrapper that splits work by row-blocks over a `ThreadPool`.


## Transpose
`transpose.h` contains a SIMD-friendly 8x8 transpose kernel.
Used as a helper for layout flip work around GEMM / attention.


## Noise

`noise_table.h` is a prebuilt aligned table of Gaussian floats.
Deterministic seed. Used as a cheap noise source without re-running RNG every call.

## Atomic math

`atomic_math.h` provides `atomicAdd(float)` using `std::atomic_ref<float>`.
Used for sum into a shared buffer cases


## Small config structs

- `rrelu_config.h` holds the RReLU configuration (alpha range + training flag).
- `gdn_params.h` holds GDN parameter storage (beta/gamma shapes).

