# Qt Stable Diffusion (QSd)

`qt-stable-diffusion` is the native Stable Diffusion component of the QtAI framework. It provides Qt and QML wrappers around the `stable-diffusion.cpp` runtime, allowing image generation pipelines to be configured entirely through Qt objects while executing inference in native C++.

Rather than exposing the underlying C API directly, the library models every major Stable Diffusion configuration structure as a QObject. Context creation, sampling configuration, LoRAs, embeddings, caching, tiling, guidance, image generation parameters, and output images are represented by Qt classes that integrate naturally with QML, property bindings, serialization, and the Qt object system.

Like the rest of QtAI, every parameter object derives from `QSdBaseParam`, providing built-in JSON and YAML serialization. This allows complete inference sessions to be saved, restored, edited, version controlled, or exchanged without requiring custom serialization code.

The library is intended to make native Stable Diffusion development feel like working with any other Qt module. Parameters are configured declaratively, snapshots are converted into the native `stable-diffusion.cpp` structures only when execution begins, and generated images are returned directly into `QSdImage` objects that integrate with the Qt Scene Graph for immediate display inside QML applications.

## Features

Current functionality includes:

* Native image generation using `stable-diffusion.cpp`
* Qt and QML wrappers for nearly every public inference parameter
* Context configuration through `QSdCtxParams`
* Image generation configuration through `QSdImgGenParams`
* Sampler, scheduler, guidance, cache, tiling and Hi-Res parameter objects
* LoRA and embedding configuration
* Native `QSdImage` objects capable of loading, saving and rendering images directly in the Qt Scene Graph
* Progress and logging callbacks integrated with Qt signals
* JSON and YAML serialization of all parameter objects
* Asynchronous image generation using QtConcurrent

## Architecture

The library separates inference into several layers.

### Context

`QSD` is the primary runtime singleton responsible for managing Stable Diffusion execution. It owns the native inference context, manages callbacks, launches background generation tasks, and exposes progress information to Qt and QML.

### Parameters

Every inference option is represented by its own QObject wrapper.

Examples include:

* `QSdCtxParams`
* `QSdImgGenParams`
* `QSdSampleParams`
* `QSdGuidanceParams`
* `QSdCacheParams`
* `QSdHiResParams`
* `QSdTilingParams`
* `QSdPmParams`
* `QSdLora`
* `QSdEmbedding`

Each object mirrors the corresponding native C structure while remaining fully accessible from QML.

### Images

`QSdImage` is both a native image wrapper and a `QQuickItem`.

It provides:

* loading and saving images
* automatic conversion between `QImage` and `sd_image_t`
* direct Scene Graph rendering
* serialization support
* use as generation inputs or outputs

The same object may be used for:

* generated images
* init images
* masks
* ControlNet images
* reference images

## QML Integration

A complete image generation pipeline can be configured directly from QML.

```qml
QSD.ContextParams.diffusionModelPath = "..."
QSD.ImageGenerationParams.prompt = "A red fox in the snow"

QSD.generateImage(outputImage)
```

Since every parameter is exposed as a QObject, standard Qt property bindings, animations, state machines, and settings pages can manipulate inference parameters without requiring custom glue code.

## Serialization

Every parameter class inherits from `QSdBaseParam`.

This provides built-in support for:

* JSON export/import
* YAML export/import

Entire inference configurations can therefore be stored as reusable presets or restored later without manually copying individual settings.

## Backend Support

Backend selection is inherited from `stable-diffusion.cpp`.

Depending on the build configuration, supported execution backends include:

* CPU
* CUDA
* Vulkan
* OpenCL

Backend availability depends on both the build options selected when compiling QtAI and the capabilities of the host system.

## Current Status

`qt-stable-diffusion` is currently **alpha software**.

The core image generation pipeline is functional and is already capable of driving native Stable Diffusion inference from Qt and QML. Several areas—including video generation, backend management, and some advanced parameter wrappers—are still under active development and may change as both QtAI and `stable-diffusion.cpp` evolve.

The long-term goal is to provide a complete native Qt framework for image, video, audio, and multimodal generation that integrates naturally with the rest of the QtAI ecosystem, including `QLlama`, Hugging Face integration, GGML tooling, model management, and future session orchestration.
