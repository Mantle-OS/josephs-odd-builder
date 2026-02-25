##############################
# DEPS
##############################
# TODO add the Qt Qml deps for the protoplanarty viewer

# * DEBIAN based install the following
# Note: tested on debian sid(atm)
# sudo apt install libssl-dev libzstd-dev pkgconf libpkgconf-dev zlib1g-dev libsodium-dev libflatbuffers-dev flatbuffers-compiler nlohmann-json3-dev libyaml-cpp-dev libmsgpack-cxx-dev libcatch2-dev

# for the qt stuff
# sudo apt install qt6-base-dev libqt6gui6 qt6-declarative-dev qt6-3d-dev qt6-quick3d-dev qt6-graphs-dev \
#    qml6-module-qtquick3d qml6-module-qtquick-layouts qml6-module-qtquick-controls qml6-module-qtgraphs qml6-module-qtcore qml6-module-qtqml

# * ARCH LINUX based(not tested)
# sudo pacman -S openssl zstd pkgconf zlib libsodium flatbuffers nlohmann-json yaml-cpp msgpack-cxx catch2

# * REDHAT / FEDORA / CENTOS based (not tested)
# sudo dnf install openssl-devel libzstd-devel pkgconf-pkg-config zlib-devel libsodium-devel flatbuffers-devel flatbuffers-compiler json-devel yaml-cpp-devel msgpack-devel catch2-devel

include(GNUInstallDirs)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

if(JOB_CUDA)
    find_package(CUDAToolkit REQUIRED)
    set(JOB_CUDA_LIBS
        CUDA::cudart
        CUDA::cublas
        CUDA::cusparse
        CUDA::curand
    )
endif()

## CRYPTO
pkg_check_modules(LibOpenSSL REQUIRED libssl)
pkg_check_modules(LibCrypto  REQUIRED libcrypto)
pkg_check_modules(LibZstd    REQUIRED libzstd)
pkg_check_modules(LibZ       REQUIRED zlib)
pkg_check_modules(LibSodium  REQUIRED libsodium)

## UART
pkg_check_modules(LibUdev REQUIRED libudev)

## DATA fun
pkg_check_modules(Flatbuffers  REQUIRED flatbuffers)
pkg_check_modules(NlohmannJson REQUIRED nlohmann_json)
pkg_check_modules(YAMLCpp      REQUIRED yaml-cpp)

## Tests
pkg_check_modules(CatchTwo REQUIRED catch2-with-main)
