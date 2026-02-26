# Job SIMD

SIMD of job 

This module builds as `jobsimd`.

A single “SIMD backend” type with:
- lane types (`f32` vector + `i32` vector)
- common float ops (add/sub/mul/div, min/max, abs, comparisons, blends)
- basic conversions (float <-> int)
- a few fast math approximations (exp/log/sin show up in the AVX path)

The intent is to keep intrinsics out of the higher layers.

## Provider

simd_provider selects the active backend at compile time. 

see cmake/check_hardware.cmake

CMake toggles:
- `HAS_AVX` -> x86 path
- `HAS_NEON` -> ARM path

x86 path:
- AVX2 implementation is present (`simd_avx.h`)
- provider has an AVX512 include hook

ARM path:
- NEON implementation is not done atm

The selected backend is exposed as a `SIMD` alias.

## Backends
### AVX (x86)
`AVX_F` is the float backend:
- 256-bit vectors
- 8 float lanes

### NEON (ARM)
`NEON_F` is the float backend:
- 128-bit vectors
- 4 float lanes

## Rounding mode
Defines a small `RoundingMode` enum that maps to the backend’s rounding controls.
Used by the vector helpers that need “round/floor/ceil/truncate” behavior.
