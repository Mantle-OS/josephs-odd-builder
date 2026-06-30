# QAiUtils

`qt-ai-utils` provides common utility classes used throughout the QtAI libraries.

It contains helper classes for application paths, filesystem operations, networking, asynchronous process execution, downloading, and a small collection of convenience macros used by the rest of the QtAI project.

`qt-ai-utils` is intended to be a dependency of other QtAI libraries rather than a standalone application framework.

If you are looking for the QML interface, see [qml-ai-utils.md](qml-ai-utils.md).

---

# Design

The library provides common infrastructure shared across the QtAI project.

Rather than each library implementing its own filesystem helpers, download manager, JSON API client, or process execution logic, these services are collected into a single reusable library.

Most QtAI libraries depend on `qt-ai-utils`.

---

# Dependencies

`qt-ai-utils` depends on:

* Qt Core
* Qt Network
* Qt Concurrent

---

# Library Layout

## QAiUtils

`QAiUtils` provides common application and filesystem helpers.

It defines the standard directory layout used throughout QtAI, including locations for configuration files, models, checkpoints, packages, plugins, logs, runtime files, and user data.

It also provides convenience functions for:

* Creating directories
* Reading and writing text files
* Testing for file and directory existence
* Initializing the default QtAI directory structure

Applications will normally call `createDefaultDirs()` during startup before working with the remaining QtAI libraries.

---

## QJsonApiClient

`QJsonApiClient` provides a JSON-based HTTP client built on top of Qt Network.

Requests are queued internally and return `QFuture<QJsonObject>`, allowing applications to perform network operations without blocking the calling thread.

The class supports:

* GET, POST, PUT, DELETE, and custom request methods
* JSON request bodies
* URL query parameters
* Configurable request headers
* Multiple authentication modes
* Redirect limits

This class is intended to be the common JSON API client shared by QtAI libraries.

---

## QDownloader

`QDownloader` provides asynchronous file downloading.

Downloads are queued internally and processed concurrently. Individual downloads return `QFuture<bool>` while progress and completion are reported through Qt signals.

Features include:

* Download queue management
* Configurable concurrent downloads
* Progress reporting
* Download speed reporting
* Download cancellation
* Automatic output directory management

By default, downloaded files use the standard QtAI directory layout provided by `QAiUtils`.

---

## QConcurrentProcess

`QConcurrentProcess` provides queued execution of external programs.

Applications add work items consisting of a working directory, executable, and argument list. Tasks are executed asynchronously and report completion, output, and errors through Qt signals.

This class is intended for situations where an application needs to execute external tools without blocking the user interface.

---

## Property and Pointer Macros

`qt-ai-utils` also provides a small collection of convenience macros used throughout the QtAI libraries.

These headers generate common `Q_PROPERTY` patterns, including:

* Read/write properties
* Read-only properties
* Constant properties
* Required properties
* Pointer properties
* Properties that automatically emit `save()`

These macros are intended primarily for QtAI library development to reduce repetitive QObject boilerplate.

---

# See Also

* [QtAI Overview](docs/qtai/qtai.md)
* [QML AI Utils](docs/qtai/qml-ai-utils.md)
