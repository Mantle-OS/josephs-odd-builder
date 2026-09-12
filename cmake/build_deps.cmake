# Note: tested on debian sid(only)
# see the Dockerfile for all build and runtime deps including testing against thingies for benchmarks and what not.
include(GenerateExportHeader)
include(GNUInstallDirs)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

## CRYPTO
pkg_check_modules(LibOpenSSL REQUIRED libssl)
pkg_check_modules(LibCrypto  REQUIRED libcrypto)
pkg_check_modules(LibZstd    REQUIRED libzstd)
pkg_check_modules(LibZ       REQUIRED zlib)
pkg_check_modules(LibSodium  REQUIRED libsodium)

## UART (used in both job_serial and job_usb)
pkg_check_modules(LibUdev REQUIRED libudev)

## USB (job_usb)
pkg_check_modules(LibUsb-1.0 REQUIRED libusb-1.0)

## DATA fun
pkg_check_modules(Flatbuffers  REQUIRED flatbuffers)
pkg_check_modules(NlohmannJson REQUIRED nlohmann_json)
pkg_check_modules(YAMLCpp      REQUIRED yaml-cpp)

## Sound land madness
pkg_check_modules(LibAlsa           REQUIRED alsa)
pkg_check_modules(LibSPA            REQUIRED libspa-0.2)
pkg_check_modules(LibPipewire       REQUIRED libpipewire-0.3)
pkg_check_modules(LibWirePlumber    REQUIRED wireplumber-0.5)

## Sound land codec madness
pkg_check_modules(LibOpus        REQUIRED opus)
pkg_check_modules(LibFLAC        REQUIRED flac)
pkg_check_modules(LibOgg         REQUIRED ogg)
pkg_check_modules(LibVorbis      REQUIRED vorbis vorbisenc vorbisfile)
pkg_check_modules(LibWavPack     wavpack)
if(LibWavPack_FOUND)
    add_compile_definitions(JOB_HAS_WAVPACK=1)
endif()


## more ai
if(JOB_CUDA)
    find_package(CUDAToolkit REQUIRED)
    set(JOB_CUDA_LIBS
        CUDA::cudart
        CUDA::cublas
        CUDA::cusparse
        CUDA::curand
    )
endif()

## Tests
pkg_check_modules(CatchTwo REQUIRED catch2-with-main)

