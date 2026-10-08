# Note: tested on debian sid(only)
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

## UART (used in both job_serial and job_usb, job_render)
set(HAS_UDEV OFF)
pkg_check_modules(LibUdev libudev)
if(${LibUdev_FOUND})
    set(HAS_UDEV ON)
    add_compile_definitions(JOB_HAS_UDEV=1)

endif()
if(JOB_BUILD_UART OR JOB_BUILD_INPUT_LIBINPUT OR JOB_BUILD_INPUT_USB AND NOT ${LibUdev_FOUND})
    message("Your Current Build setup needs LibInput please install the development package")
endif()

## USB (job_usb)
pkg_check_modules(LibUsb-1.0 REQUIRED libusb-1.0)

## DATA fun
pkg_check_modules(Flatbuffers  REQUIRED flatbuffers)
pkg_check_modules(NlohmannJson REQUIRED nlohmann_json)
pkg_check_modules(YAMLCpp      REQUIRED yaml-cpp)

## Sound land madness
if(JOB_BUILD_SOUND)
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
endif()


set(HAS_LIBINPUT OFF)
pkg_check_modules(LibInput libinput)
if(${LibInput_FOUND})
    set(HAS_LIBINPUT ON)
    add_compile_definitions(JOB_HAS_LIBINPUT=1)
endif()

set(HAS_LIBEVDEV OFF)
pkg_check_modules(LibEvdev evdev)
if(${LibEvdev})
    set(HAS_LIBEVDEV ON)
    add_compile_definitions(JOB_HAS_LIBEVDEV=1)
endif()


## Start Graphics
if(JOB_BUILD_OPENGL)
    pkg_check_modules(OpenGl REQUIRED opengl)
    pkg_check_modules(GlesV2 REQUIRED glesv2)
    # glproto.pc
    add_compile_definitions(JOB_HAS_OPENGL=1)
endif()

if(JOB_BUILD_VULKAN)
    pkg_check_modules(VulkanLoader REQUIRED vulkan)
    # pkg_check_modules(       REQUIRED SPIRV-Headers.pc
    add_compile_definitions(JOB_HAS_VULKAN=1)
endif()

if(JOB_BUILD_RENDER)
    set(HAS_RENDER_BACKEND   OFF)
    set(HAS_DRM              OFF)
    set(HAS_DRI              OFF)
    set(HAS_EGL              OFF)
    set(HAS_GBM              OFF)
    set(HAS_3D_GFX           OFF)
    set(HAS_SIMPLE_KMS       OFF)
    set(HAS_FULL_KMS         OFF)
    # set(HAS_WAYLAND         OFF)


    pkg_check_modules(XkbCommon REQUIRED xkbcommon)

    # FIXME come back to this after job_input is a real thing.
    if(NOT HAS_LIBINPUT)
        message(FATAL_ERROR "JOB_BUILD_RENDER currently needs libinput and friends")
        # pkg_check_modules(LibInput  REQUIRED libinput)
    endif()

    pkg_check_modules(FreeType  REQUIRED freetype2)
    pkg_check_modules(Harfbuzz  REQUIRED harfbuzz)

    if(JOB_BUILD_RENDER_FB)
        add_compile_definitions(JOB_FB=1)
        set(HAS_RENDER_BACKEND ON)
     endif()


    if(JOB_BUILD_OPENGL OR JOB_BUILD_VULKAN OR JOB_BUILD_RENDER_KMS OR JOB_BUILD_RENDER_WAYLAND)
        pkg_check_modules(LibDrm libdrm)
        if(LibDrm_FOUND)
            set(HAS_DRM ON)
            # add_compile_definitions(JOB_DRM=1)
        else()
            message(FATAL_ERROR "JOB_BUILD_OPENGL JOB_BUILD_VULKAN JOB_BUILD_RENDER_WAYLAND JOB_BUILD_RENDER_KMS Require Linux DRM")
        endif()

        pkg_check_modules(LibDri dri)
        if(LibDri_FOUND)
            set(HAS_DRI ON)
            # add_compile_definitions(JOB_DRI=1)
        endif()

        pkg_check_modules(LibEgl egl)
        if(LibEgl_FOUND)
            set(HAS_EGL ON)
            # add_compile_definitions(JOB_EGL=1)
        endif()

        pkg_check_modules(LibGbm gbm)
        if(LibGbm_FOUND)
            set(HAS_GBM ON)
            # add_compile_definitions(JOB_GBM=1)
        endif()

        if(HAS_EGL AND HAS_GBM AND HAS_DRM)
            set(HAS_3D_GFX ON)
            # add_compile_definitions(JOB_3D_GFX=1)
        endif()

    endif()

    if(JOB_BUILD_RENDER_KMS)
        if(HAS_DRM)
            set(HAS_SIMPLE_KMS ON)
            add_compile_definitions(JOB_SIMPLE_KMS=1)
            set(HAS_RENDER_BACKEND ON)
        endif()
        if(HAS_3D_GFX)
            set(HAS_FULL_KMS ON)
            add_compile_definitions(JOB_FULL_KMS=1)
            set(HAS_RENDER_BACKEND ON)
        endif()
    endif()

    if(JOB_BUILD_RENDER_WAYLAND AND HAS_3D_GFX)
        pkg_check_modules(XWayland          REQUIRED xwayland)
        pkg_check_modules(WaylandClient     REQUIRED wayland-client)
        pkg_check_modules(WaylandCursor     REQUIRED wayland-cursor)
        pkg_check_modules(WaylandServer     REQUIRED wayland-server)

        pkg_check_modules(WaylandEgl        REQUIRED wayland-egl)

        pkg_check_modules(WaylandProtocols  REQUIRED wayland-protocols)
        pkg_get_variable(JOB_WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

        set(JOB_WAYLAND_SCANNER  "" CACHE STRING "Where the wayland scanner is located")
        if(NOT JOB_WAYLAND_SCANNER)
            pkg_check_modules(WaylandScanner    REQUIRED wayland-scanner)
            pkg_get_variable(JOB_WAYLAND_SCANNER wayland-scanner wayland_scanner)
        else()
            # for cross compile hints
            if(EXISTS JOB_WAYLAND_SCANNER)
                message("wayland-scanner found")
            else()
                message(FATAL_ERROR "Could not find wayland-scanner in /usr/bin")
            endif()
        endif()

        set(JOB_WAYLAND_LIBS
            ${XWayland_LIBRARIES}
            ${WaylandClient_LIBRARIES}
            ${WaylandCursor_LIBRARIES}
            ${WaylandEgl_LIBRARIES}
            ${WaylandServer_LIBRARIES}
        )

        set(HAS_RENDER_BACKEND ON)

    endif()

    if(NOT HAS_RENDER_BACKEND)
        message(FATAL_ERROR "JOB_BUILD_RENDER is set but there is no backend set")
    endif()

endif()

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

