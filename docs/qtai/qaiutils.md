# QtAI Utils

`qt-ai-utils` is the foundation library of the QtAI ecosystem. It provides the common utilities used throughout the rest of the QtAI libraries, including filesystem helpers, application paths, network helpers, asynchronous process execution, and download support.

Unlike the higher-level QtAI libraries, `qt-ai-utils` does not provide AI functionality directly. Instead, it supplies the common infrastructure that allows those libraries to share a consistent runtime environment.

Most QtAI libraries depend directly on `qt-ai-utils`.

---

# Library Layout

The QtAI utilities project consists of two separate libraries.

| Library          | Purpose                                                |
| ---------------- | ------------------------------------------------------ |
| **qt-ai-utils**  | Core C++ utility library used by the QtAI ecosystem.   |
| **qml-ai-utils** | QML utility module intended for Qt Quick applications. |

Although both libraries belong to the same project, they target different users.

The `qt-ai-utils` library is primarily intended for C++ developers implementing QtAI libraries, while `qml-ai-utils` provides convenience types that are useful directly from QML applications.

---

# qt-ai-utils

The core utility library provides a collection of shared classes used throughout QtAI.

## QAiUtils

`QAiUtils` defines the standard QtAI directory layout and provides common filesystem utilities.

Responsibilities include:

* Creating the standard QtAI directory structure.
* Managing application runtime paths.
* Reading and writing text files.
* Creating directories automatically.
* File and directory existence checks.
* Debugging the configured directory layout.

Rather than allowing every QtAI library to invent its own storage locations, `QAiUtils` centralizes all filesystem paths in one location.

Typical directories include:

* Configuration files
* JSON and YAML configuration storage
* Runtime files
* Log output
* Generated output
* AI models
* Diffusion checkpoints
* Text encoders
* LoRAs
* Embeddings
* ControlNet models
* VAE models
* Audio VAE models
* Packages
* Plugins
* Additional QML modules
* User data
* Session information

This allows every QtAI library to share a common directory layout without duplicating configuration logic.

---

## QJsonApiClient

`QJsonApiClient` provides a reusable JSON-based HTTP client.

Rather than implementing network requests individually, higher-level libraries can derive from or build upon this class to communicate with REST-style services.

It serves as the common networking layer for components such as Hugging Face integration and future online services.

---

## QDownloader

`QDownloader` provides generic asynchronous downloading.

It is intentionally independent of any particular AI backend and is suitable for downloading:

* AI models
* Package files
* Metadata
* Cached resources
* Configuration files

Higher-level libraries build upon this functionality instead of implementing their own download logic.

---

## QConcurrentProcess

`QConcurrentProcess` provides asynchronous external process execution.

Unlike `QProcess`, this class is designed around Qt's concurrent programming model and integrates with `QFuture` and `QPromise`.

Processes execute asynchronously while their completion status and return codes are delivered back through a queued future interface.

This makes it suitable for long-running helper utilities without blocking the application's main thread.

---

## Utility Headers

The project also contains several convenience headers.

### property-macros.h

Provides helper macros that reduce repetitive `Q_PROPERTY` boilerplate.

### pointer-macros.h

Provides convenience macros for common pointer property patterns.

These headers exist solely to reduce repetitive code throughout QtAI and should be considered implementation conveniences rather than primary public APIs.

---

# qml-ai-utils

The `qml-ai-utils` library provides utility objects intended for use directly from QML.

Current functionality includes:

* Common path objects
* Path helper utilities
* QML string list helpers
* Shared object model utilities

This library exists to expose frequently used utility functionality to QML without requiring applications to implement their own wrappers.

---

# Dependencies

`qt-ai-utils` depends on:

* Qt Core
* Qt Network
* Qt Concurrent

`qml-ai-utils` additionally depends on:

* Qt GUI
* Qt Quick
* Qt QML

---

# Design Goals

The QtAI utility libraries are designed around a few simple principles.

* Provide common functionality once.
* Avoid duplicate implementations across QtAI libraries.
* Establish a consistent filesystem layout.
* Offer reusable asynchronous infrastructure.
* Keep higher-level QtAI libraries focused on AI functionality rather than application infrastructure.

As QtAI grows, additional shared functionality should generally be added here before being duplicated elsewhere in the ecosystem.
