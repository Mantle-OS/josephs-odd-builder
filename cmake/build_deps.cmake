##############################
# DEPS (build_deps.cmake)
##############################
# TODO add the Qt Qml deps for the protoplanarty viewer

# * DEBIAN based install the following
# Note: tested on debian sid(atm)
# sudo apt install libssl-dev libzstd-dev pkgconf libpkgconf-dev zlib1g-dev libsodium-dev libflatbuffers-dev flatbuffers-compiler nlohmann-json3-dev libyaml-cpp-dev libmsgpack-cxx-dev libcatch2-dev libggml-dev

# for the nvidia stuff.(you need the upsteam nvida repo)
# sudo apt install  libcusparse-13-1 libcusparse-dev-13-1  cublas13 libcublas13-dev-cuda-13 cuda-cudart-13-1 cuda-cudart-dev-13-1 libcurand-13-1 libcurand-dev-13-1

# for the qt stuff
# sudo apt install qt6-base-dev libqt6gui6 qt6-declarative-dev qt6-3d-dev qt6-quick3d-dev qt6-graphs-dev \
#    qml6-module-qtquick3d qml6-module-qtquick-layouts qml6-module-qtquick-controls qml6-module-qtgraphs qml6-module-qtcore qml6-module-qtqml

# * ARCH LINUX based(not tested)
# sudo pacman -S openssl zstd pkgconf zlib libsodium flatbuffers nlohmann-json yaml-cpp msgpack-cxx catch2

# * REDHAT / FEDORA / CENTOS based (not tested)
# sudo dnf install openssl-devel libzstd-devel pkgconf-pkg-config zlib-devel libsodium-devel flatbuffers-devel flatbuffers-compiler json-devel yaml-cpp-devel msgpack-devel catch2-devel

include(GenerateExportHeader)
include(GNUInstallDirs)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

## FIXME much latyer add a section for JOB_WINDOWS

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

## Sound land
# sudo apt-get install libwireplumber-0.5-dev libspa-0.2-dev pipewire-alsa libpipewire-0.3-dev libspa-0.2-jack libspa-0.2-bluetooth libspa-0.2-modules libasound2-dev
pkg_check_modules(LibAlsa           REQUIRED alsa)
pkg_check_modules(LibSPA            REQUIRED libspa-0.2)
pkg_check_modules(LibPipewire       REQUIRED libpipewire-0.3)
pkg_check_modules(LibWirePlumber    REQUIRED wireplumber-0.5)

## Sound land Codecs
# sudo apt-get install -y libopus-dev libflac-dev libogg-dev libvorbis-dev  libwavpack-dev
pkg_check_modules(LibOpus        REQUIRED opus)
pkg_check_modules(LibFLAC        REQUIRED flac)
pkg_check_modules(LibOgg         REQUIRED ogg)
pkg_check_modules(LibVorbis      REQUIRED vorbis vorbisenc vorbisfile)
pkg_check_modules(LibWavPack     wavpack)
if(LibWavPack_FOUND)
    add_compile_definitions(JOB_HAS_WAVPACK=1)
endif()
## End Sound

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

## we build this now
# pkg_check_modules(LibGgml REQUIRED ggml)
# find_library(LibGgmlBase NAMES libggml-base ggml-base)
# if(LibGgmlBase)
#     message(STATUS "Found GGML Base: ${LibGgmlBase}")
# else()
#     message(FATAL_ERROR "LibGgmlBase not found!")
# endif()

## Tests
pkg_check_modules(CatchTwo REQUIRED catch2-with-main)

if(JOB_QT)
    # stop warning me about working plugins......
    # set(QT_SKIP_AUTO_PLUGIN_INCLUSION OFF)
    set(QT_SKIP_AUTO_QML_PLUGIN_INCLUSION OFF)
    find_package(Qt6 6.2 COMPONENTS
        Core
        Gui
        Network
        Concurrent
        Qml
        Quick
        QuickControls2
        REQUIRED
    )

    set(QML_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/qt6/qml")

    qt_policy(SET QTP0001 NEW)
    qt_policy(SET QTP0004 NEW)


    if(TARGET Qt6::qtquick2plugin)
        message(STATUS "Qt6::qtquick2plugin exists")
    else()
        message(FATAL_ERROR "Qt6::qtquick2plugin DOES NOT exist")
    endif()

    message(STATUS "QT6_IS_SHARED_LIBS_BUILD = ${QT6_IS_SHARED_LIBS_BUILD}")
    # message(FATAL_ERROR "BUILD_SHARED_LIBS = ${BUILD_SHARED_LIBS}")
endif()
