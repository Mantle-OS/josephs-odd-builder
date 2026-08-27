# Joseph's Odd Builder

Joseph's Odd Builder, or **JOB**, is a C++23/26 GNU/Linux playground for building userland libraries, runtimes, tools, simulations, and applications.

The project is split into two major documentation areas:

## JOB Core Ecosystem

Low-level runtime libraries for threading, IO, networking, SIMD, crypto, serialization, ANSI/TUI, science simulation, and experimental AI.

* [JOB documentation](docs/job/job.md)

## QtAI Ecosystem

Qt/QML-facing AI infrastructure built on top of JOB and native runtimes such as `llama.cpp`, `stable-diffusion.cpp`, libsodium, zstd, and Hugging Face APIs.

* [QtAI documentation](docs/qtai/qtai.md)

## Building

Only tested in linux


```bash
sudo apt install gcc-16
sudo apt install libssl-dev libzstd-dev pkgconf libpkgconf-dev zlib1g-dev \
    libsodium-dev libflatbuffers-dev nlohmann-json3-dev \
    libyaml-cpp-dev libmsgpack-cxx-dev libcatch2-dev \
    libcusparse-13-3 libcusparse-dev-13-3  cublas13 libcublas13-dev-cuda-13 \
    cuda-cudart-13-3 cuda-cudart-dev-13-3 libcurand-13-3 libcurand-dev-13-3 \
    cuda-libraries-13-3  cuda-runtime-13-3 cuda-opencl-13-3 \
    qt6-base-dev libqt6gui6 qt6-declarative-dev qt6-3d-dev qt6-quick3d-dev qt6-graphs-dev \
    qml6-module-qtquick3d qml6-module-qtquick-layouts qml6-module-qtquick-controls \
    qml6-module-qtgraphs qml6-module-qtcore qml6-module-qtqml \
    libwireplumber-0.5-dev libspa-0.2-dev pipewire-alsa libpipewire-0.3-dev \
    libspa-0.2-jack libspa-0.2-bluetooth libspa-0.2-modules libasound2-dev \
    libopus-dev libflac-dev libogg-dev libvorbis-dev  libwavpack-dev
```

Now you can export the CUDA details 

```bash
set(JOB_CUDA_TOOLKIT_ROOT_DIR    "/usr/local/cuda-13.3"                CACHE STRING "Where the cuda toolkit is ")
set(JOB_CUDA_COMPILER            "/usr/local/cuda-13.3/bin/nvcc"       CACHE STRING "full path to the nvcc compiler")
set(JOB_CUDA_HOST_COMPILER       "/usr/bin/g++-16"                     CACHE STRING "the compiler that nvcc is compiled with")
set(JOB_CUDA_ARCHITECTURES       "120"                                 CACHE STRING "version of the cuda api")
```

I would not change the compiler. JOB_CUDA_HOST_COMPILER as a lot of things depend on c++26

All other build options can be found over here [cmake build options](cmake/build_options.cmake)
 
From here it is the normal cmake stuff.

```bash
mkdir build
cd build
cmake --build ../ --target all
```


## Status

This is an experimental systems playground. APIs are allowed to change while the architecture is still being shaped.

* SIMD: 512 and Neon has fallen behind as the full api has not been fully discovered yet. 
* Win32 has NEVER been compiled. There are some things in place but its pretty much Gnum/Linux only at this point for the native job libs
* ggml, sdcpp and llama have hardcoded values atm if you need to change then they are in  
  * [GGML](cmake/cmake_ggml_build_flags.cmake)
  * [Llama cpp](cmake/cmake_llama_cpp_build_flags.cmake)
  * [stable_diffusion cpp](cmake/cmake_stable_diffusion_cpp_build_flags.cmake)
* The code is young'ish there are bound to be issues though we do test things. [see section 11](docs/coding_style.md)   
* job_model and job_token are changing almost everyday at this point. but this should settle down at some point.

