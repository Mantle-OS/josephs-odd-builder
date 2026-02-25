# Job Science

The science stack of job.

`jobscience` links against `job_core`, `job_threads`, `job_net`, `job_crypto`.

NOTE (alpha reality): `libs/job_science/CMakeLists.txt` has big chunks commented out.
Right now the build mainly ships **data + solvers**. Frames/engine/integrators exist in-tree but are not wired into the library target.

job:
- `job::science::data` = structs used by the solvers (particles, disk model, zones, forces)
- `job::science::solvers` = integrators + N-body + stencil grids
- `job::threads` = ThreadPool + `parallel_for` used everywhere here

---

## Build switches

- `JOB_SCIENCE_USE_MMAP` (OFF by default): enables mmap-based frame reading paths (only matters once frames are wired back in)
- double precision option exists but is currently commented out

---

## Data (`job::science::data`)

Small'ish POD-style types used by the solvers:

- `Vec3f` : float 3D vector utility type (math ops, length, etc)
- `Particle` : simulation particle (id, position/velocity/acceleration, radius/mass, temperature, composition, zone)
- `Composition` : material-ish properties (density, emissivity, heat capacity, etc)
- `DiskModel` / `DiskModelUtil` : disk geometry + unit conversions (AU <-> meters, etc)
- `Forces` : force helpers (spring force shows up in integrator tests)
- `Zones` / reminder of disk zoning (`DiskZone`) : region tagging for particles

This layer is intentionally plain. It’s the “stuff that moves around” layer.

---

## Solvers (`job::science::solvers`)

### Time integration

- velocity-verlet integrator (KDK/DKD style) driven by:
  - user-provided accessors for position/velocity/acceleration
  - an accel calculator callback that fills acceleration for the current particle state
  - optional threaded loops via `job::threads` parallel helpers

- Euler integrator (simple explicit step)
- RK4 integrator (scratch-buffer based, same accel-calc model)

### N-body gravity / long-range interactions

- Barnes–Hut:
  - tree build + approximate far-field force accumulation
  - compared against direct O(N²) force in tests

- FMM (Fast Multipole Method):
  - kernel math + coefficient structs
  - P2M / M2L / L2P style transforms
  - engine wrapper used in tests with trait hooks (position/mass/applyForce)

### Stencils / grids

- 2D and 3D stencil grids:
  - double-buffer stepping (read old state, write new state)
  - boundary modes used in tests (wrap/clamp)
  - simple CA-style use shows up in tests (Game of Life kernel)

---

## Stuff that exists but is currently commented out in the build

THIS IS OLD AND I ADDED THIS CODE. 

### Frames (`libs/job_science/frames/*`)
Frame header + serializer/deserializer + sink/source adapters for IO and net.
The code is present, but the CMake target has it commented out.

### Engine (`libs/job_science/engine/*`)
A higher-level simulation engine wrapper that pulls together:
- particle state
- disk/zones model
- threading algo selection enums
- frame read/write plumbing

Also present, also commented out in the build target.

### Integrators folder (`libs/job_science/integrators/*`)

---

## Tests

`tests/job_science` is mostly solver validation:
- Verlet integrator behavior (KDK/DKD)
- Euler integrator sanity
- RK4 integrator sanity
- Barnes–Hut vs direct force checks + edge cases
- FMM kernel integrity checks + approximations
- Stencil grids (double buffering + boundary behavior + Game of Life oscillator)

Threading in tests uses `job::threads` sched contexts (stealing/sporadic/fifo) to exercise “same math, different scheduler”.
