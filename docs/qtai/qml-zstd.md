# QML Zstd

`qml-zstd` provides a QML interface to the `qt-zstd` library.

It exposes the asynchronous compression engine and the blocking compressor and decompressor as QML types for use in Qt Quick applications.

If you are looking for the C++ library, see [qzstd.md](qzstd.md).

---

# Design

`qml-zstd` is a thin QML layer built on top of `qt-zstd`.

Unlike some QtAI libraries, very little additional abstraction is required because the underlying C++ classes already expose their functionality through Qt properties, signals, and invokable methods.

The QML types primarily register the existing C++ classes as QML elements and expose a small number of convenience functions where appropriate.

---

# Dependencies

`qml-zstd` depends on:

* Qt Core
* Qt Quick
* Qt Quick Controls
* Qt QML
* `qt-zstd`

When `qt-zstd` is built with sodium support enabled, the same functionality is available through the QML interface.

---

# Library Layout

## QmlZstd

`QmlZstd` is the primary QML interface to the asynchronous compression engine.

It inherits from `QZstd` and exposes the same asynchronous compression and decompression workflows to QML.

In addition, it provides the `hasSodium` property so applications can determine at runtime whether encryption and signing support are available.

`QmlZstd` is registered as a QML singleton.

---

## QmlCompressor

`QmlCompressor` provides access to the blocking compression worker.

It inherits from `QZstdCompressor` and adds a `compress()` convenience function for QML applications.

This type is intended for applications that require synchronous compression.

---

## QmlDecompressor

`QmlDecompressor` provides access to the blocking decompression worker.

It inherits from `QZstdDecompressor` and adds a `decompress()` convenience function for QML applications.

This type is intended for applications that require synchronous decompression.

---

# Runtime Features

Applications can query the `hasSodium` property to determine whether the library was built with sodium support.

When available, the underlying `QZstd` functionality includes authenticated encryption, authenticated decryption, archive signing, and signature verification.

---

# See Also

* [QtAI Overview](docs/qtai/qtai.md)
* [QZstd](docs/qtai/qzstd.md)
