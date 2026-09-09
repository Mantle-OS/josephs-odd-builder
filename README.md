# Joseph's Odd Builder

**C++26 · GNU/Linux Only \***

Joseph's Odd Builder, or **JOB**, is an experimental C++26 GNU/Linux project for building
userland libraries, runtimes, tools, simulations, and applications.

JOB targets GNU/Linux only. It does not pretend to support Windows or provide a generic
cross-platform operating-system abstraction layer.

> \* Native JOB C++ code is C++26. CUDA/GGML `.cu` translation units remain C++23 where
> required by NVIDIA's NVCC toolchain.

## Documentation

The documentation is split by subsystem.

### JOB

* [JOB overview](docs/job/job.md)
* [Threading](docs/job/threading_overview.md)
* [I/O](docs/job/io_overview.md)
* [Networking](docs/job/net_overview.md)
* [UART](docs/job/uart_overview.md)
* [Crypto](docs/job/crypto_overview.md)
* [ANSI](docs/job/ansi_overview.md)
* [TUI](docs/job/tui_overview.md)
* [Sound](docs/job/sound_overview.md)
* [Science](docs/job/science_overview.md)
* [AI](docs/job/ai.md)
* [AI benchmarks](docs/job/ai_benchmarks.md)
* [Token library](docs/job/job_token.md)
* [Serializer](docs/job/serializer_overview.md)
* [Zstd](docs/job/zstd_overview.md)

### GGML

* [job_ggml overview](docs/job_ggml/overview.md)
* [Backends](docs/job_ggml/backend.md)
* [Contexts](docs/job_ggml/context.md)
* [Devices](docs/job_ggml/devices.md)
* [GGUF](docs/job_ggml/gguf.md)
* [Operations](docs/job_ggml/operations.md)
* [Optimizer](docs/job_ggml/optimizer.md)
* [Quantization](docs/job_ggml/quant.md)
* [Tensors](docs/job_ggml/tensors.md)

### JobSchema

* [JobSchema design introduction](docs/job_schema/design/intro.md)
* [JobSchema design](docs/job_schema/design/job_schema_design.md)
* [JobSchema dependencies](docs/job_schema/design/job_schema_design_deps.md)
* [JobSchema generator design](docs/job_schema/design/job_schema_gen_design.md)
* [JobSchema generator example](docs/job_schema/design/job_schema_gen_example_design.md)
* [JobSchema design tests](docs/job_schema/design/test_job_schema_design.md)

## Toolchain

JOB uses C++26 and currently targets GCC 16.

On Debian-based systems:

```bash
sudo apt-get update
sudo apt-get install gcc-16 g++-16 cmake ninja-build pkg-config git
```

Optionally configure GCC 16 as the system alternative:

```bash
sudo update-alternatives \
    --install /usr/bin/gcc gcc /usr/bin/gcc-16 100 \
    --slave /usr/bin/g++ g++ /usr/bin/g++-16

sudo update-alternatives --set gcc /usr/bin/gcc-16
```

The CI/CD environment builds JOB from the repository's
[Debian container](docker/Dockerfile). That file is the authoritative reference for the
complete package set used by CI.

## Configure and Build

Clone the repository and initialize its submodules:

```bash
git clone https://github.com/Mantle-OS/josephs-odd-builder.git
cd josephs-odd-builder
git submodule update --init --recursive
```

Configure a Release build:

```bash
cmake -S . -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release
```

Build:

```bash
cmake --build build -j$(nproc)
```

Run the tests:

```bash
ctest --test-dir build --output-on-failure
```

## CUDA

CUDA support is optional and controlled by the `JOB_CUDA` build option.

The currently tested CUDA toolchain is CUDA 13.3 with GCC 16 as the NVCC host compiler.

The CUDA toolchain can be configured through the following CMake cache variables:

| Option                      | Default                         | Description                                      |
| --------------------------- | ------------------------------- | ------------------------------------------------ |
| `JOB_CUDA_TOOLKIT_ROOT_DIR` | `/usr/local/cuda-13.3`          | Root directory of the CUDA toolkit installation. |
| `JOB_CUDA_COMPILER`         | `/usr/local/cuda-13.3/bin/nvcc` | Full path to the NVIDIA CUDA compiler.            |
| `JOB_CUDA_HOST_COMPILER`    | `/usr/bin/g++-16`               | Host C++ compiler used by NVCC.                   |
| `JOB_CUDA_ARCHITECTURES`    | `120a`                          | CUDA architecture target.                        |

For example:

```bash
cmake -S . -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DJOB_CUDA_ARCHITECTURES=120a
```

`JOB_CUDA_HOST_COMPILER` should remain GCC 16.

NVCC currently requires `--allow-unsupported-compiler` for this host compiler
combination. JOB adds that internally where required.

See NVIDIA's
[feature-set compiler targets](https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/compute-capabilities.html#feature-set-compiler-targets)
for architecture values.

## Build-Time Options

JOB exposes CMake options for reducing builds, isolating subsystems, CI, benchmarks,
and optional CUDA support.

These options are primarily development controls. They are not promises that every
combination forms a valid independent configuration; disabling a lower-level subsystem
can also require disabling its dependents.

For the complete and authoritative list, see
[cmake/build_options.cmake](cmake/build_options.cmake).

GGML, llama.cpp, and stable-diffusion.cpp build configuration is kept under `cmake/`:

* [GGML build flags](cmake/cmake_ggml_build_flags.cmake)
* [llama.cpp build flags](cmake/cmake_llama_cpp_build_flags.cmake)
* [stable-diffusion.cpp build flags](cmake/cmake_stable_diffusion_cpp_build_flags.cmake)

## Qt

Qt-facing libraries and applications live in the separate
[QtJob](https://github.com/Mantle-OS/QtJob) repository.

The native JOB repository does not require Qt.

## Status

JOB is pre-alpha and under active development.

There is currently no ABI or API compatibility promise. Experimental code, build
options, libraries, and internal architecture may change or disappear while the project
is being shaped.

`job_model`, `job_token`, and `job_schema` are changing particularly quickly.

## License
Job is licensed under the GPLv3 or a commercial license if needed 
