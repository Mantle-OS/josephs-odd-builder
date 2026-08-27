ARG DEBIAN_TAG=sid-slim
FROM debian:sid-slim

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y locales && \
    echo "en_US.UTF-8 UTF-8" > /etc/locale.gen && \
    locale-gen
ENV LANG=en_US.UTF-8 LANGUAGE=en_US.UTF-8 LC_ALL=en_US.UTF-8

ENV CUDA_HOME=/usr/local/cuda-13.3
ENV TORCH_CUDA_ARCH_LIST="13.3"

ENV PATH=${CUDA_HOME}/bin:${PATH}
ENV LD_LIBRARY_PATH=${CUDA_HOME}/lib64:${LD_LIBRARY_PATH}

RUN apt-get update && \
    apt-get install --no-install-recommends -y \
        gcc-16 g++-16 build-essential cargo \
        chrpath wget curl git wget ca-certificates git cmake ninja-build pkg-config \
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
        qml6-module-qtquick3d qml6-module-qtquick-layouts qml6-module-qtquick-controls \
        qml6-module-qtgraphs qml6-module-qtcore qml6-module-qtqml \
        libwireplumber-0.5-dev libspa-0.2-dev pipewire-alsa libpipewire-0.3-dev \
        libspa-0.2-jack libspa-0.2-bluetooth libspa-0.2-modules libasound2-dev \
        libopus-dev libflac-dev libogg-dev libvorbis-dev  libwavpack-dev \
        libgl1 libglib2.0-0 \
        libudev-dev \
        libopenblas-dev libblas3 && \
    rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-16 100 --slave /usr/bin/g++ g++ /usr/bin/g++-16 && \
    update-alternatives --set gcc /usr/bin/gcc-16

RUN wget https://developer.download.nvidia.com/compute/cuda/repos/debian12/x86_64/cuda-keyring_1.1-1_all.deb && \
    dpkg -i cuda-keyring_1.1-1_all.deb && \
    echo "deb [signed-by=/usr/share/keyrings/cuda-archive-keyring.gpg trusted=yes] https://developer.download.nvidia.com/compute/cuda/repos/debian12/x86_64/ /" > /etc/apt/sources.list.d/cuda-debian12-x86_64.list && \
    apt-get update && \
    apt-get install --no-install-recommends -y \
        cuda-nvcc-13-3 \
        cuda-libraries-dev-13-3 cuda-libraries-13-3  cuda-runtime-13-3 \
        libcublas-dev-13-3 cublas13 libcublas13-dev-cuda-13 \
        libcusparse-13-3 libcusparse-dev-13-3  \
        cuda-cudart-13-3 cuda-cudart-dev-13-3 \
        libcurand-13-3 libcurand-dev-13-3 \
        cuda-opencl-dev-13-3 cuda-opencl-13-3 && \
    rm cuda-keyring_1.1-1_all.deb

RUN update-alternatives --install /usr/bin/nvcc nvcc /usr/local/cuda-13.3/bin/nvcc 100 && \
    update-alternatives --set nvcc /usr/local/cuda-13.3/bin/nvcc

ADD entrypoint.sh /entrypoint.sh
ENTRYPOINT ["/bin/bash", "/entrypoint.sh"]