# Qml AI Utils

`qml-ai-utils` provides common QML components used throughout the QtAI libraries.

It contains reusable models and utility classes that simplify building Qt Quick applications. Rather than each library implementing its own models or exposing C++ helpers individually, these common pieces are collected into a shared library.

`qml-ai-utils` depends on `qt-ai-utils` and is intended for QML-facing applications.

---

# Design

The library provides common QML infrastructure shared across QtAI.

Its primary purpose is to expose common data models and helper classes that can be reused by multiple libraries and applications.

---

# Dependencies

`qml-ai-utils` depends on:

* Qt Core
* Qt Gui
* Qt Quick
* Qt Qml
* Qt Network
* Qt Concurrent
* qt-ai-utils

---

# Library Layout

## ObjectListModel

`ObjectListModel` is the primary model used throughout the QtAI QML libraries.

It provides a `QAbstractListModel` implementation for lists of `QObject`-derived classes. Model roles are generated from Qt properties, allowing QML views to bind directly to object properties.

The model supports common operations such as adding, removing, moving, clearing, and retrieving objects while keeping the QML view synchronized with changes.

It is intended for applications where a C++ backend owns a collection of objects that are presented or edited from QML.

---

## QmlStringList

`QmlStringList` provides a QML-friendly string model.

It is intended for controls that display or edit lists of strings, including:

* ComboBox
* ListView
* Repeater
* Menu
* Completion models

Unlike `QStringListModel`, `QmlStringList` exposes common container operations directly to QML. Strings can be appended, prepended, moved, removed, searched, and cleared without additional C++ wrapper classes.

Typical uses include:

* Model selection
* Checkpoint lists
* LoRA lists
* Prompt presets
* Configuration values
* Recently opened files

---

## QAiPathing

`QAiPathing` is a QML singleton for scanning common QtAI model directories.

It maintains search directories and available files for common AI asset types, including:

* Checkpoints
* Diffusion models
* Text encoders
* LoRAs
* Embeddings
* ControlNet models
* Upscale models
* VAE models
* Audio VAE models

Each collection is exposed through `QmlStringList`, allowing it to be used directly by QML controls such as `ComboBox` and `ListView`.

Calling `scanAll()` refreshes every asset list and emits completion signals for each category followed by a final `scanDone()` signal.

---

## QAiPath

`QAiPath` is a small QML model for applications that need to manage a single model path.

It provides a current selection together with a list of search directories and available models. It is intended for user interfaces that only need a single model category instead of the full `QAiPathing` scanner.

---

# Notes

Some classes in this library may move into future session or package-management libraries as the QtAI architecture evolves.

---

# See Also

* [QtAI Overview](docs/qtai/qtai.md)
* [QAi Utils](docs/qtai/qt-ai-utils.md)
