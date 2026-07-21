# Job SIMD

Vector math primitives for job, kept behind one backend-agnostic interface so callers never write `_mm256_*`/`vaddq_f32` intrinsics directly.

job_simd has no dependencies beyond the compiler's own intrinsics headers (`<immintrin.h>` on x86, `<arm_neon.h>` on ARM). It has no Qt dependency, does no heap allocation of its own, and every function is `inline`/`static inline`, there is nothing to link against beyond whatever consumes the headers.

- `job::simd` provides a single `SIMD` alias, resolved at compile time to whichever backend the build is configured for, so the same call site (`SIMD::add`, `SIMD::exp`, `SIMD::pull`) works unchanged whether the target is AVX2 or (eventually) NEON
- every operation in this library is available at one of three usage tiers, Scalar, Vector, or Tile, and which one you reach for changes what you're actually buying: see "How to actually use this library" below before writing new code against this module
- benchmark numbers below are on a Intel Core i9-14900K (Raptor Lake, AVX2 + FMA + AVX-VNNI, no AVX-512 on this die); every number quoted in this document came from that hardware, not a theoretical peak

job_simd has no dependencies beyond the compiler's own intrinsics headers (`<immintrin.h>` on x86, `<arm_neon.h>` on ARM). It has no Qt dependency, does no heap allocation of its own, and every function is `inline`/`static inline`, there is nothing to link against beyond whatever consumes the headers.

- `job::simd` provides a single `SIMD` alias, resolved at compile time to whichever backend the build is configured for, so the same call site (`SIMD::add`, `SIMD::exp`, `SIMD::pull`) works unchanged across backends
- every operation in this library is available at one of three usage tiers, Scalar, Vector, or Tile, and which one you reach for changes what you're actually buying: see "How to actually use this library" below before writing new code against this module

## Backend status

Work in progress — the API surface across backends won't be fully settled until AVX2/AVX-VNNI usage across the rest of the codebase has actually exercised every call, at which point the other backends get the same calls added to reach parity.

| Backend | Width | CMake flag | Header | Status |
|---|---|---|---|---|
| AVX (plain) | — | `JOB_AVX_FLAG` | none yet | maybe later if ever need on real old chips |
| AVX2 | 8 | `JOB_AVX_2_FLAG` | `simd_avx.h` | implemented, shares the AVX-VNNI backend below |
| AVX-VNNI | 8 | `JOB_AVX_VNNI_FLAG` (default **ON**) | `simd_avx.h` | **dogfooded**, on an i9-14900K |
| AVX-512 / AVX-512 VNNI | 16 | `JOB_AVX_512_FLAG` / `JOB_AVX_512_VNNI_FLAG` | `simd_avx512.h` | written, substantially complete, never run on real hardware — this dev machine has no AVX-512 |
| NEON | 4 | `JOB_AVX_NEON_FLAG` | `simd_arm.h` | written for a demo, untouched for ~6 months, not benchmarked or dogfooded |

Every number quoted anywhere in this document came from the AVX-VNNI row, on the hardware named above.

## Rounding

### rounding_mode.h
Defines `RoundingMode`, a small enum (`Nearest`/`Down`/`Up`/`Truncate`) mapped to each backend's own rounding control values (`_MM_FROUND_*` on x86, plain integers on ARM pending a real NEON implementation).

- exists purely so `SIMD::round<Mode>()` call sites read the same regardless of backend, callers never see `_MM_FROUND_TO_NEAREST_INT` or its ARM equivalent directly
- the AVX-specific `#include <immintrin.h>` in this header lives outside the `job::simd` namespace block, not inside it — intrinsics headers declare their contents at global scope, and if this header were ever the first thing in a translation unit to pull them in, an include inside the namespace would silently nest every intrinsic under `job::simd` instead, breaking unqualified calls elsewhere in that TU

## Provider

### simd_provider.h
The single entry point for this library. Selects the active backend at compile time (see `cmake/check_hardware.cmake` for the `HAS_AVX`/`HAS_AVX_512`/`HAS_NEON` toggles), aliases `SIMD` to that backend's type, and wires the three named math entry points, `SIMD::exp`, `SIMD::log`, `SIMD::exp_fast`, to specific implementations from `simd_math.h`.

- everything that needs this library should include `simd_provider.h`, not the backend headers or `simd_math.h` directly, `simd_provider.h`'s own include order depends on being the thing that pulls `simd_math.h` in, and including `simd_math.h` 
- `SIMD::exp` currently maps to `exp_estrin`; `exp_poly5` (a Horner-scheme alternative with equivalent accuracy) exists in `simd_math.h` but has no dispatch entry point of its own yet

## Backends

### AVX (x86) — `simd_avx.h`
`AVX_F` is the concrete float backend when the build targets AVX2: 256-bit vectors (`f32` = `__m256`), 8 float lanes per register (`SIMD::width() == 8`).

- every primitive here is a thin, direct wrapper over a single intrinsic (`add` -> `_mm256_add_ps`, `mul_plus`-> `_mm256_fmadd_ps` when `__FMA__` is defined, a manual multiply-add otherwise) 
the point of this file is that nothing above it needs to know or care which instruction actually ran
- `f16` here is a naming leftover, it is `__m128` (four packed `float`s, the low or high half of an `f32` register), not an IEEE binary16 half-float fwiw

### AVX-512 (x86) — `simd_avx512.h`
`AVX512_F` is a substantially complete backend: 512-bit vectors, 16 float lanes. Written and reasonably fleshed out, but never run on real hardware, I dont have a CPU with AVX-512. The AVX2-backend-only today time and hardware ....

### NEON (ARM) — `simd_arm.h`
`NEON_F` the float backend for ARM: 128-bit vectors, 4 float lanes. Not fully implemented yet, There are a bunch of calls that match up but not fully yet. more dogfooding AVX2 to figure out the full shape of the api.

## Core math — `simd_math.h`

Fast approximations for `exp` and `log`, built entirely on `AVX_F` primitives, no libm calls anywhere in the hot path.

- `exp_poly5` and `exp_estrin` are both degree-5 polynomial approximations over the same Cody-Waite range-reduced interval, one using a serial Horner chain, the other Estrin's scheme for shorter dependency chains. Measured worst-case relative error for both is ~2e-5 over `x ∈ [-10, 10]`. Counterintuitively, `exp_poly5` (Horner) benchmarks faster than `exp_estrin` (Estrin) on this hardware, roughly 10-15% — Estrin's instruction-level-parallelism advantage doesn't pay for its extra `r²`/`r⁴` squaring work at only degree 5. Treat this the way you'd treat a near-field/far-field crossover: real, hardware-dependent, not a bug in either function
- `exp_schraudolph` is the classic Schraudolph fast-exp bit-hack, not a polynomial at all. Measured worst-case relative error is ~3% over `x ∈ [-5, 5]`, not for anything needing precision, but it is the fastest of the three by a wide margin
- `avx_log` returns a quiet NaN for `x <= 0` rather than throwing; its `noexcept` is honest only because of that branch, not because the signature independently guarantees no-throw behavior
- all three exp variants and `avx_log` clamp their input range internally before doing any real work (`±87.5` for the polynomials, `±87` for `exp_schraudolph`), inputs past that are still handled, just saturate rather than overflow

## Parallel iteration — `simd_for.h`

`simd_for` is a generic loop shape: call one lambda `width()` elements at a time (the "vector" step), then a second lambda for whatever's left over (the "scalar" tail). Every function in `simd_math.h` that operates on a buffer rather than a single register is meant to be driven through this, not a hand-written `for` loop.

- the three-argument overload is purely serial, no threading involved
- the pool overload chunks work into `pool.workerCount()`-sized pieces and hands each chunk to `job::threads::parallel_for`, but only above a 1024-element minimum grain; below that, or with a single-worker pool, it falls back to the serial path automatically
- the pool overload calls `parallel_for` with an explicit `grain=1`, this matters: `simd_for` pre-chunks work into `numChunks` (roughly one per worker) before calling `parallel_for`, and `parallel_for`'s own grain heuristic has no way to know each of those `numChunks` "items" secretly represents a huge amount of real work. Without forcing `grain=1`, `parallel_for` looks at a small chunk count, decides it's not worth threading by its own (correct, for its normal callers) logic, and silently runs everything on the calling thread — this was a real, previously-undetected bug in this library, caught by benchmarking a large-enough workload that parallel should have won and mysteriously didn't
- `job::threads::parallel_for` itself falls back to a plain serial loop when called from inside a worker thread already (`ThreadPool::inWorkerThread()`), so nesting `simd_for(pool, ...)` inside another pooled task does not deadlock, it just runs serially at that point

## Transpose — `transpose.h`

`transpose_kernel_8x8`/`transpose_kernel_4x4` are hand-written AVX2 transpose kernels (unpack → shuffle → permute-lanes, the standard sequence for this operation). `transpose()` is the general entry point: dispatches to the 8x8 kernel for `SIMD::width()`-sized blocks, falls back to scalar element-by-element copies for any remaining rows/columns that don't fill a full block.

- the `if constexpr (K == 8)` dispatch inside `transpose()` is a known, acknowledged gap: if `SIMD::width()` is ever 16 (a real AVX-512 backend), this still routes into the 4-wide kernel instead of a 16-wide one, silently wrong output, not just a missed optimization. Flagged in the source, not yet fixed
- naive scalar transpose on this hardware measures well below realistic memory bandwidth (~3 GB/s against a much higher ceiling), it is latency-bound, not bandwidth-bound: every strided write touches a fresh cache line and stalls. The SIMD kernel's unpack/shuffle/permute sequence exists specifically to write in cache-line-respecting bursts, which is most of where its ~6x real-world win over naive scalar comes from, not raw compute

## How to actually use this library

There isn't one right way to call into `job_simd`, there are three, and which one you want depends on what you're doing, not personal preference. 
Every number below is from the same i9-14900K (AVX2+FMA), 1,000,000-element workloads unless noted.

**Scalar** — a plain loop, `std::` functions, no `job_simd` at all. This is your baseline, not something to ship, but useful to know your actual floor.

**Vector** — call `SIMD::` functions directly against a `std::vector<float>`/raw buffer, `width()` lanes at a time (`SIMD::pull`/`SIMD::mov`). This is the normal way to use the library, buffer in, buffer out, no special setup.

**Tile** — pull data straight into a small block of `f32` registers (a `width()`x`width()` tile), operate register-to-register with no memory round-trip until the very end, then store back once. This is what the AVX2 micro-kernels in `gemm.h` and the transpose kernels above already do internally. It's more code to write by hand, and it's how you get the rest of the performance the library has to offer.

**Kernel — recommended.** If you've spent any real time in AVX, this is the actual destination: write a dedicated microkernel (`transpose_kernel_8x8` in `transpose.h` is an example in this codebase) 
and drive it from your own tiling loop not from a naive per-tile wrapper. This is where Tile's remaining overhead (the driver loop itself) 
actually gets amortized away, and it's how everything in this framework that's "fast".
Downside More upfront work than calling `SIMD::` functions but payoff is a massive speed up most the time.
 

### Cheap ops: the win is almost entirely memory residency

| `mul_plus` | time | throughput |
|---|---|---|
| Scalar | 211.95us | 4.72 Gelem/s |
| Vector | 226.91us | 4.41 Gelem/s |
| Tile | 71.61us | 13.97 Gelem/s |

Scalar and Vector land in the same place here, on purpose, not a bug: at `-O3`, the compiler auto-vectorizes a trivial branch-free scalar loop into the same instructions `SIMD::mul_plus` emits by hand. 
For an op this cheap, the compiler is already doing your job for you. Tile pulls ahead (~3.2x over Vector) 
because it never round-trips the data through memory between load and store, everything stays in registers/L1 for the duration of the tile.

Same story for `mask-select` (`gt_ps`+`and_ps`, the exact pattern used for eps-guards in force calculations):

| `mask-select` | time | throughput |
|---|---|---|
| Scalar | 217.80us | 4.59 Gelem/s |
| Vector | 245.16us | 4.08 Gelem/s |
| Tile | 70.09us | 14.27 Gelem/s |

### Expensive ops: the win is mostly algorithmic, and shows up even at the Vector tier

| `exp` | time | throughput | vs `std::exp` |
|---|---|---|---|
| `std::exp` (Scalar) | 1.767ms | 566 Melem/s | 1x |
| `exp_estrin` (Vector) | 352.41us | 2.84 Gelem/s | ~5x |
| `exp_estrin` (Tile) | 201.65us | 4.96 Gelem/s | ~8.8x |
| `exp_schraudolph` (Vector) | 221.01us | 4.52 Gelem/s | ~8x |
| `exp_schraudolph` (Tile) | 67.35us | 14.85 Gelem/s | **~26x** |

`exp_estrin`/`avx_log` do real range reduction and polynomial evaluation with actual branching, an auto-vectorizer can't reconstruct that from a `std::exp` call the way it reconstructs a trivial FMA, 
so the Scalar-to-Vector jump here is a genuine algorithmic win. Notice the Vector-to-Tile gap *shrinks* as the function gets more compute-heavy: `mul_plus` gained ~3.2x from Tile, `exp_estrin` only ~1.75x, 
`avx_log` only ~1.4x. Tile's advantage is specifically about avoiding memory traffic, the more time an op spends computing versus moving data, the less that advantage matters. 
`exp_schraudolph` sits at the cheap end of that spectrum and gets the full Tile bonus on top of its already-large algorithmic win, 
hence the ~26x total, at the cost of accuracy (~3% relative error, not for anything needing precision).

### Rule of thumb

If you're calling a cheap op (`mul_plus`, comparisons, basic arithmetic) over a large buffer, hand-writing a Tile kernel is worth it, that's most of the win available. 
If you're calling `exp`/`log`/anything transcendental, Vector alone already gets you most of the algorithmic win over `std::`, 
Tile is a smaller, still-real bonus on top, worth it if you're already structuring code around tiles for other reasons, not necessarily worth restructuring code just to get it.

### Threading only pays off at real scale

`simd_for`'s pool overload (backed by `job::threads::parallel_for`) only wins once there's enough work to amortize per-task dispatch overhead. 
At 4,000,000 elements of `exp` work, serial and parallel are close to a wash. 
At 64,000,000 elements, parallel pulls ahead by roughly 1.9x on an 8-thread pool (measured 10.3ms-11.5ms parallel vs 22.1ms serial across runs, noisier than the single-threaded numbers 
above once real cores are contending for memory bandwidth). Don't reach for the pool overload on small batches, `simd_for`'s own single-threaded path is already fast; save threading for genuinely large workloads.

### Utils available for testing/benchmarking against this library

Just like everything else in job there is a test for that. 