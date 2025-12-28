# Job AI Cords

Views and Extents for `job::ai`.

Small header-only layer: extents + row-major views + a few convenience wrappers (1D..4D).
No ownership here, just pointers + shape.

job:
- `job::ai::cords` = extents + views + aligned weights container
- `job::science::data` = used by `ml_particle.h` (Vec3 bridge)

## Extents

`Extents<MaxRank>` holds shape with a logical size (`rank`) up to 4.

- stores dims in a fixed array (zero-filled)
- `rank()` / `size()` is the number of active dims
- `volume()` is the product of active dims (0-safe cases exist)

Small helpers exist for common AI shapes:
- BS, BSD, BSDH

## View
`View<T>` is a non-owning row-major view over contiguous storage.

Holds:
- `T *data`
- `Extent` (rank up to 4)
- computed strides (last dimension contiguous)

Core:
- linear access by flat index
- N-D access with bounds checks (asserts)
- `reshape()` keeps the same storage and changes the shape (volume must match)
- `slice(i)` drops the first dimension and returns a view into the i-th slab


## View iteration
`ViewIter` walks slices along the first dimension.

- `begin()/end()` on a view returns slices
- free helpers `beginSlices()/endSlices()` exist too
- each deref returns a lightweight `View` (copy of pointer + new extent)

convenience for “batch” style loops.

## Named wrappers

Thin wrappers around `ViewR` for common ranks:

- `Fiber`  : rank 1
- `Matrix` : rank 2
- `Volume` : rank 3
- `Batch`  : rank 4

These add small rank-checked helpers (`rows/cols`, etc) and keep the intent readable.

There is also a templated `Rank<N>` wrapper for “pick rank at compile time”.


## Layout helpers

`layout.h` defines a `LayoutType` enum and a tiny `offset2D` helper for:
- row-major
- column-major

Strided/tiled layouts are declared but not implemented in the 2D helper (asserts).

## Aligned weights

`AlignedAllocator<T, 64>` is used for aligned float storage.

`AiWeights` is:
- `std::vector<float, AlignedAllocator<float, 64>>`

Used as the weights container (64-byte aligned).


## Small structs

- `AttentionShape`: `{ batch, seq, dim, numHeads }`
- `Tile4 / Tile8 / Tile16`: compile-time tile descriptors (rows/cols/elements/bytes)
  - Tile8 is the “AVX2 sized” register block descriptor
  - Tile16 is the “super tile / AVX-512-ish” descriptor

---

## ML particle bridge

`ml_particle.h` provides a small adapter type for particle like data stored in float buffers:

- `MLVec3f` is a view over 3 floats with conversions to/from `job::science::data::Vec3f`
- `MLParticle` groups `pos/vel/acc + mass`

Also includes a simple N-body force loop used by adapter tests.
