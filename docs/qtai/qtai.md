# Warning

QtAI is currently an early alpha project under active development. APIs, library boundaries, and internal implementations may change as the project evolves.

---

# QtAI

Unlike JobAI  QtAi is for desktop builds and not emnbedded boards.

QtAI is a collection of modern C++ and Qt libraries for building local AI applications without requiring Python. It provides reusable libraries for inference, model management, cryptography, sessions, package management, downloads, and Qt/QML integration.

Unlike Job::AI, which targets constrained Unix-like systems and embedded AI infrastructure, QtAI targets Qt-based applications running on more capable hardware. QtAI is designed around native C++, Qt, and QML while remaining independent of Python.

Each library within QtAI has a well-defined responsibility and can generally be used independently of the others.

---

### Level 1

## QAiUtils

A Qt/C++ utility library used by the rest of QtAI.

Provides shared file/path helpers, JSON API helpers, downloader support, async process helpers, and common internal macros used by other QtAI libraries.

* [See also](docs/qtai/qaiutils.md)

---

## QmlAiUtils

A QML utility module built on top of `qt-ai-utils`.

Provides QML-facing helper objects for paths, path lists, string lists, and shared object model helpers used by QtAI applications.

* [See also](docs/qtai/qml-ai-utils.md)

## QSodium

A Qt-friendly cryptography library built on top of libsodium. Provides secure memory, password hashing, authenticated encryption, digital signatures, hashing, random number generation, and key management. QSodium is a general-purpose cryptography library and is not specific to AI applications

* [See also](docs/qtai/qsodium.md)

---

### Level 2

## QmlSodium

"A QML interface built on top of qt-sodium. Provides declarative QML elements for common cryptographic workflows including password handling, authenticated encryption, digital signatures, hashing, and key management. QmlSodium is intended for Qt Quick applications and uses the underlying qt-sodium library for all cryptographic operation

* [See also](docs/qtai/qml-sodium.md)

---

## QZstd

A Qt/C++ library built on top of Zstandard.

Provides compression and decompression for files and directories, streaming support through `QIODevice`, and an asynchronous API for compression tasks. When built with sodium support enabled, QZstd also provides authenticated encryption and archive signing.

* [See also](docs/qtai/qzstd.md)


## QmlZstd

A QML interface built on top of `qt-zstd`.

Provides QML access to asynchronous and blocking compression workflows, including file and directory compression and decompression. When built with sodium support enabled, the same encryption and signing features provided by `qt-zstd` are also available.

* [See also](docs/qtai/qml-zstd.md)


### Level 3

## QHuggingFace (QHF)

Provides Qt and QML classes for working with the Hugging Face Hub.

Includes support for the Hugging Face REST API, local package metadata, repository information, user management, and package caching. QHF integrates with the QtAI infrastructure libraries for networking, secure credential storage, downloads, and local package management.

* [See also](docs/qtai/qhuggingface.md)

---

## QLlama

Provides Qt and QML classes for integrating `llama.cpp` into Qt applications.

Includes support for model loading, execution contexts, tokenization, sampling, LoRA adapters, local inference, OpenAI-compatible client and server engines, and JSON/YAML serialization of runtime configuration through `QLlamaBase`.

* [See also](docs/qtai/qllama.md)

---

## Qt Stable Diffusion (QSD)

Provides Qt and QML classes for integrating `stable-diffusion.cpp` into Qt applications.

Includes support for image generation pipelines, context management, sampler configuration, LoRA and embedding control, guidance and conditioning systems, caching and tiling strategies, backend device selection, and runtime parameter serialization via QSdBaseParam (JSON and YAML).

QSD exposes a fully declarative inference model where generation parameters are composed in QML and executed natively in C++, with results streamed back into QSdImage objects for direct Scene Graph rendering.

* [See also](docs/qtai/qstable-diffusion.md)

---

### Level 4

## Schema Libraries

QtAI also contains schema-based libraries used for package management, trust, and session management.

These libraries define data contracts rather than user-facing APIs.

* [See also](docs/qtai/aipkg-schema.md)


## Session Management

QtAI provides session management libraries responsible for user configuration, vault management, application initialization, and runtime session state.

These libraries provide the foundation for authentication, package ownership, and user-specific configuration throughout the QtAI ecosystem.

* [See also](docs/qtai/qsession.md)

---
