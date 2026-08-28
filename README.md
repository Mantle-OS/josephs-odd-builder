# Joseph's Odd Builder

Joseph's Odd Builder, or **JOB**, is a C++26(minus nvidia cu stuff at 23) GNU/Linux playground for building userland libraries, runtimes, tools, 
simulations, and applications.

The project is split into 2 major documentation areas kinda... 

## JOB Core Ecosystem

Low-level runtime libraries for threading, IO, networking, SIMD, crypto, serialization, ANSI/TUI, science simulation, and experimental AI.

* [JOB documentation](docs/job/job.md)

## QtAI Ecosystem

Qt/QML-facing AI infrastructure built on top of JOB and native runtimes such as `llama.cpp`, `stable-diffusion.cpp`, libsodium, zstd, and Hugging Face APIs.

* [QtAI documentation](docs/qtai/qtai.md)

## Building

JOB is currently tested on Linux.

The CI/CD environment builds JOB from a [debian sid "slim" container](docker/Dockerfile), 
so the following packages represent the dependencies for **all of job**.

### Dependencies

Update the package index and install the required development packages:

```bash
sudo apt-get update
sudo apt-get install \
    gcc-16 g++-16 build-essential cargo \
    chrpath wget curl git ca-certificates cmake ninja-build pkg-config \
    glslc libvulkan-dev libvulkan1 spirv-tools spirv-headers mesa-vulkan-drivers \
    libwebp-dev \
    libssl-dev libzstd-dev \
    pkgconf libpkgconf-dev \
    zlib1g-dev \
    libsodium-dev \
    libflatbuffers-dev \
    libmsgpack-cxx-dev \
    libcatch2-dev \
    nlohmann-json3-dev \
    libyaml-cpp-dev \
    qt6-base-dev \
    libqt6gui6 qt6-declarative-dev qt6-3d-dev qt6-quick3d-dev qt6-graphs-dev \
    qml6-module-qtquick3d \
    qml6-module-qtquick-layouts \
    qml6-module-qtquick-controls \
    qml6-module-qtgraphs \
    qml6-module-qtcore \
    qml6-module-qtqml \
    libwireplumber-0.5-dev \
    libspa-0.2-dev \
    pipewire-alsa \
    libpipewire-0.3-dev \
    libspa-0.2-jack \
    libspa-0.2-bluetooth \
    libspa-0.2-modules \
    libasound2-dev \
    libopus-dev \
    libflac-dev \
    libogg-dev \
    libvorbis-dev \
    libwavpack-dev \
    libgl1 \
    libglib2.0-0 \
    libudev-dev \
    libopenblas-dev \
    libblas3
```

### optional update-alternatives for gcc

configure the system alternatives so `gcc` and `g++` resolve to GCC 16:

```bash
sudo update-alternatives \
    --install /usr/bin/gcc gcc /usr/bin/gcc-16 100 \
    --slave /usr/bin/g++ g++ /usr/bin/g++-16

sudo update-alternatives --set gcc /usr/bin/gcc-16
```


## NVIDIA CUDA

CUDA is optional and controlled by the `JOB_CUDA` build option.

The currently tested CUDA toolchain is CUDA 13.3 with GCC 16 as the NVCC host compiler.

### Install CUDA via Nvidia's repository 

```bash
cd /tmp
wget https://developer.download.nvidia.com/compute/cuda/repos/debian12/x86_64/cuda-keyring_1.1-1_all.deb
sudo dpkg -i cuda-keyring_1.1-1_all.deb
echo "deb [signed-by=/usr/share/keyrings/cuda-archive-keyring.gpg trusted=yes] https://developer.download.nvidia.com/compute/cuda/repos/debian12/x86_64/ /"  | sudo tee /etc/apt/sources.list.d/cuda-debian12-x86_64.list    
sudo apt-get update
sudo apt-get install \
    cuda-nvcc-13-3 \
    cuda-libraries-dev-13-3 \
    cuda-libraries-13-3 \
    cuda-runtime-13-3 \
    libcublas-dev-13-3 \
    cublas13 \
    libcublas13-dev-cuda-13 \
    libcusparse-13-3 \
    libcusparse-dev-13-3 \
    cuda-cudart-13-3 \
    cuda-cudart-dev-13-3 \
    libcurand-13-3 \
    libcurand-dev-13-3 \
    cuda-opencl-dev-13-3 \
    cuda-opencl-13-3
rm -f /tmp/cuda-keyring_1.1-1_all.deb
```

Optionally configure it through `update-alternatives`:

```bash
sudo update-alternatives --install /usr/bin/nvcc nvcc /usr/local/cuda-13.3/bin/nvcc 100
sudo update-alternatives --set nvcc /usr/local/cuda-13.3/bin/nvcc
```

## CUDA Build Options

The CUDA toolchain used by JOB can be configured through the following CMake cache variables.

| Option                      | Default                         | Description                                      |
| --------------------------- | ------------------------------- | ------------------------------------------------ |
| `JOB_CUDA_TOOLKIT_ROOT_DIR` | `/usr/local/cuda-13.3`          | Root directory of the CUDA toolkit installation. |
| `JOB_CUDA_COMPILER`         | `/usr/local/cuda-13.3/bin/nvcc` | Full path to the NVIDIA CUDA compiler.           |
| `JOB_CUDA_HOST_COMPILER`    | `/usr/bin/g++-16`               | Host C++ for NVCC. Don't change this             |
| `JOB_CUDA_ARCHITECTURES`    | `120a`                          | CUDA GPU architecture to compile for.            |


`JOB_CUDA_ARCHITECTURES` is the option most users are likely to need to change. 
The correct value depends on the NVIDIA GPU being targeted.

For example, `120a` targets supported Blackwell GPUs and enables architecture-specific features associated with that target.
[see also Nvidia's feature set compiler targets](https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/compute-capabilities.html#feature-set-compiler-targets)

The value can be overridden when configuring the build:

```bash
cmake -S . -B build \
    -DJOB_CUDA_ARCHITECTURES=120a
```

> Do not change `JOB_CUDA_HOST_COMPILER` It should Always remain GCC 16. 
JOB uses C++26,  while al ```.cu``` files(in GGML) remain constrained 
by NVCC compiler support (only up to c++23). Nvidia also says that GCC 16 is not fully supported 
so we add ```--allow-unsupported-compiler``` internally to get around this build time odyssey 
of all nvcc calls(ggml only) using c++23 while the rest of the project uses the bleeding edge of c++23 

## Configure and Build

Clone the repository including its submodules:

```bash
git clone https://github.com/Mantle-OS/josephs-odd-builder.git
cd josephs-odd-builder
git submodule update --init --recursive
```

From here it is the normal cmake stuff.

Example:
```bash
mkdir build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target all -j$(nproc)
```

## JOB Build-Time Options

JOB exposes a large number of CMake build-time options. These are primarily intended for developers who need to reduce the build, isolate a subsystem, or work on individual libraries.

These options are powerful, but they are **not independent feature flags**. Disabling a low-level library can break anything above it in the dependency graph.

For example:

```text
job_simd
   |
   V
job_core
   |
   V
many other JOB libraries
```

Disabling `JOB_BUILD_SIMD` while leaving `JOB_BUILD_CORE` enabled will therefore produce an invalid configuration. The same applies to other dependency relationships throughout the project.

If the goal is simply to disable an entire category of functionality, prefer one of the global guard options:

| Option     | Default | Description                                                     |
| ---------- | ------- | --------------------------------------------------------------- |
| `JOB_CUDA` | `ON`    | Enables NVIDIA CUDA support and CUDA-dependent portions of JOB. |
| `JOB_QT`   | `ON`    | Enables Qt 6 support and Qt-based libraries/applications.       |
| `JOB_APPS` | `ON`    | Builds supported example and utility applications.              |

Use the more granular `JOB_BUILD_*` options when actively developing or testing individual components.

### CI and Benchmark Options

| Option                | Default | Description                                                                                                                                 | Documentation |
| --------------------- | ------: | ------------------------------------------------------------------------------------------------------------------------------------------- | ------------- |
| `JOB_CI_BUILD`        |   `OFF` | Enables settings specific to the CI/CD environment. Defines `JOB_CI_BUILD` and limits CI-specific thread usage with `JOB_CI_MAX_THREADS=4`. | —             |
| `JOB_TEST_BENCHMARKS` |    `ON` | Enables Catch2 benchmarks when building in `Release` mode.                                                                                  | —             |

### Core JOB Libraries

| Option              | Default | Description                                                              | Documentation                                        |
| ------------------- | ------: | ------------------------------------------------------------------------ | ---------------------------------------------------- |
| `JOB_BUILD_SIMD`    |    `ON` | Builds the `job_simd` SIMD and vectorized math library.                  | [SIMD overview](docs/job/simd_overview.md)           |
| `JOB_BUILD_CORE`    |    `ON` | Builds the `job_core` foundational library.                              | [Core overview](docs/job/core_overview.md)           |
| `JOB_BUILD_THREADS` |    `ON` | Builds the `job_threads` threading, scheduling, and concurrency library. | [Threading overview](docs/job/threading_overview.md) |
| `JOB_BUILD_CRYPTO`  |    `ON` | Builds the `job_crypto` cryptography library.                            | [Crypto overview](docs/job/crypto_overview.md)       |
| `JOB_BUILD_ZSTD`    |    `ON` | Builds the `job_zstd` compression and transport library.                 | [Zstd overview](docs/job/zstd_overview.md)           |
| `JOB_BUILD_IO`      |    `ON` | Builds the `job_io` I/O library. Currently Linux-dependent.              | [I/O overview](docs/job/io_overview.md)              |
| `JOB_BUILD_UART`    |    `ON` | Builds the `job_uart` serial/UART library. Currently Linux-dependent.    | [UART overview](docs/job/uart_overview.md)           |
| `JOB_BUILD_NET`     |    `ON` | Builds the `job_net` networking library.                                 | [Networking overview](docs/job/net_overview.md)      |
| `JOB_BUILD_SCIENCE` |    `ON` | Builds the `job_science` scientific simulation and solver library.       | [Science overview](docs/job/science_overview.md)     |
| `JOB_BUILD_AI`      |    `ON` | Builds the `job_ai` CPU-oriented AI/inference library.                   | [AI overview](docs/job/ai.md)                        |
| `JOB_BUILD_ANSI`    |    `ON` | Builds the `job_ansi` terminal/ANSI processing library.                  | [ANSI overview](docs/job/ansi_overview.md)           |
| `JOB_BUILD_TUI`     |    `ON` | Builds the `job_tui` terminal UI library.                                | [TUI overview](docs/job/tui_overview.md)             |
| `JOB_BUILD_SOUND`   |    `ON` | Builds the `job_sound` audio pipeline and Linux audio stack support.     | [Sound overview](docs/job/sound_overview.md)         |

### Model and Token Libraries

| Option            | Default | Description                                               | Documentation                                  |
| ----------------- | ------: | --------------------------------------------------------- | ---------------------------------------------- |
| `JOB_BUILD_GGML`  |    `ON` | Builds the JOB wrapper and abstraction layer around GGML. | [job_ggml overview](docs/job_ggml/overview.md) |
| `JOB_BUILD_TOKEN` |    `ON` | Builds the `job_token` tokenizer library.                 | [Token library](docs/job/job_token.md)         |
| `JOB_BUILD_MODEL` |    `ON` | Enables the `job_model` model runtime library.            | —                                              |

### Serializer and AI Package Libraries

| Option                             |                        Default | Description                                           | Documentation                                          |
| ---------------------------------- | -----------------------------: | ----------------------------------------------------- | ------------------------------------------------------ |
| `JOB_BUILD_SERIALIZER`             |                           `ON` | Builds the `job_serializer` serialization framework.  | [Serializer overview](docs/job/serializer_overview.md) |
| `JOB_BUILD_SERIALIZER_MSGPACK`     |                           `ON` | Enables the MessagePack serializer backend.           | [Serializer overview](docs/job/serializer_overview.md) |
| `JOB_BUILD_AIPKG_SCHEMA`           | `JOB_BUILD_SERIALIZER_MSGPACK` | Builds the generated AI package schema libraries.     | [QAI schema](docs/qtai/qai-schema.md)                  |
| `JOB_BUILD_AIPKG`                  | `JOB_BUILD_SERIALIZER_MSGPACK` | Builds the `job_aipkg` AI package and ledger library. | [AI package schema](docs/qtai/qai-schema/aipkg.md)     |
| `JOB_BUILD_SERIALIZER_FLATBUFFERS` |                          `OFF` | Enables the FlatBuffers serializer backend.           | [Serializer overview](docs/job/serializer_overview.md) |

The generated AI package schema is currently divided into several areas:

* [AI package schema](docs/qtai/qai-schema/aipkg.md)
* [AI ledger schema](docs/qtai/qai-schema/ailedger.md)
* [AI session schema](docs/qtai/qai-schema/aisession.md)

### Qt Adapter Libraries

These options are relevant when `JOB_QT` is enabled.

| Option                      | Default | Description                                     | Documentation                                       |
| --------------------------- | ------: | ----------------------------------------------- | --------------------------------------------------- |
| `JOB_BUILD_QTAI`            |    `ON` | Enables the Qt AI adapter library group.        | [QtAI overview](docs/qtai/qtai.md)                  |
| `JOB_BUILD_QAIUTILS`        |    `ON` | Builds the Qt AI utility adapter library.       | [QAI Utils](docs/qtai/qaiutils.md)                  |
| `JOB_BUILD_QSODIUM`         |    `ON` | Builds the Qt/libsodium adapter library.        | [QSodium](docs/qtai/qsodium.md)                     |
| `JOB_BUILD_QZSTD`           |    `ON` | Builds the Qt/Zstd adapter library.             | [QZstd](docs/qtai/qzstd.md)                         |
| `JOB_BUILD_QSD`             |    `ON` | Builds the Stable Diffusion Qt adapter library. | [QStable Diffusion](docs/qtai/qstable-diffusion.md) |
| `JOB_BUILD_QLLAMA`          |    `ON` | Builds the llama.cpp Qt adapter library.        | [QLlama](docs/qtai/qllama.md)                       |
| `JOB_BUILD_QHF`             |   `OFF` | Builds the Hugging Face Qt adapter library.     | [QHuggingFace](docs/qtai/qhuggingface.md)           |
| `JOB_BUILD_QSESSIONMANAGER` |   `OFF` | Builds the Qt session manager.                  | —                                                   |

Related QML adapter documentation is also available for:

* [QML AI Utils](docs/qtai/qml-ai-utils.md)
* [QML Sodium](docs/qtai/qml-sodium.md)
* [QML Zstd](docs/qtai/qml-zstd.md)

### Example

Individual options can be changed at configure time:

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DJOB_BUILD_SOUND=OFF \
    -DJOB_BUILD_QHF=OFF
```

Again, when disabling individual libraries, make sure their dependents are also disabled or otherwise removed from the build.

For the complete and authoritative list of build options, see [cmake/build_options.cmake](cmake/build_options.cmake).

### Llama, GGML and stable diffusion options
ggml, sdcpp and llama have hardcoded values atm if you need to change then they are in  
  * [GGML build flags](cmake/cmake_ggml_build_flags.cmake)
  * [Llama cpp build flags](cmake/cmake_llama_cpp_build_flags.cmake)
  * [stable_diffusion cpp build flags](cmake/cmake_stable_diffusion_cpp_build_flags.cmake)

---

## Status

This is an experimental systems playground. APIs are allowed to change while the architecture is still being shaped.

* SIMD: 512 and Neon has fallen behind as the full api has not been fully discovered yet. 
* Win32 has NEVER been compiled. There are some things in place but its pretty much Gnum/Linux only at this point for the native job libs
* The code is young'ish there are bound to be issues though we do test things. [see section 11](docs/coding_style.md)   
* job_model and job_token are changing almost everyday at this point. but this should settle down at some point.
