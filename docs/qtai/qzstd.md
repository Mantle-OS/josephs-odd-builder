# QZstd

`qt-zstd` is a Qt/C++ library built on top of Zstandard.

It provides classes for compressing and decompressing files and directories, streaming compressed data through `QIODevice`, and asynchronous compression tasks. When built with sodium support enabled, the library also provides authenticated encryption and archive signing through `qt-sodium`.

`qt-zstd` is not AI-specific. It lives in the QtAI project because many QtAI libraries work with models, packages, datasets, and other assets that benefit from compression and integrity verification.

If you are looking for the QML interface, see [qml-zstd](docs/qtai/qml-zstd.md).

---

# Design

The library is organized into a small set of focused classes.

Low-level classes perform the compression work directly, while `QZstd` provides the asynchronous interface used by applications and the QML layer.

When sodium support is enabled, additional worker classes provide encrypted archives and detached signatures without changing the standard compression workflow.

---

# Dependencies

`qt-zstd` depends on:

* Qt Core
* Qt Concurrent
* Zstandard

Optional:

* `qt-sodium`
* libsodium

The sodium dependency is optional and is only required when encrypted archives or archive signing are enabled.

---

# Library Layout

## QZstd

`QZstd` is the main entry point for the library.

It inherits the common properties from `QZstdOptions`, owns the worker objects, and executes compression tasks asynchronously using `QtConcurrent` and `QFutureWatcher`.

Applications will normally use this class instead of the lower-level worker classes.

For standard archives it provides:

* Compression
* Decompression

When sodium support is enabled it also supports:

* Compression with signing
* Compression with encryption
* Decompression with signature verification
* Decompression with decryption

---

## QZstdOptions

`QZstdOptions` provides the common state shared by the library.

It contains properties such as:

* Input path
* Output path
* Compression level
* Progress
* Error reporting

The worker classes and the asynchronous `QZstd` engine inherit from this class so applications interact with a consistent interface.

---

## QZstdIO

`QZstdIO` is a `QIODevice` implementation for streaming compressed data.

It wraps another `QIODevice` and performs compression or decompression as data is written or read.

This class is intended for applications that need streaming support rather than file-based compression.

---

## QZstdCompressor

`QZstdCompressor` performs blocking compression operations.

It provides support for compressing:

* Individual files
* Directory trees

This class is normally used internally by `QZstd` but may also be used directly when blocking behavior is desired.

---

## QZstdDecompressor

`QZstdDecompressor` performs blocking decompression operations.

It provides support for extracting:

* Individual files
* Directory trees

Like the compressor, this class is typically used through `QZstd`.

---

## QZstdCompressorCrypto

When sodium support is enabled, `QZstdCompressorCrypto` extends the standard compressor with authenticated encryption.

Encryption keys are stored internally using `QSecureMem` and may be supplied directly or as Base64 encoded values.

---

## QZstdDecompressorCrypto

`QZstdDecompressorCrypto` extends the standard decompressor with authenticated decryption.

Encrypted archives are verified during the decryption process before the compressed data is extracted.

---

## QZstdSign

`QZstdSign` provides detached archive signing and signature verification.

It uses the signing support provided by `qt-sodium` and is used by the secure archive workflow when signing or verification has been requested.

---

# Compression Modes

Depending on the build configuration and application requirements, `qt-zstd` supports several workflows.

Standard archive:

```text
File / Directory
        │
        ▼
   Compress
        │
        ▼
      Archive
```

Encrypted archive:

```text
File / Directory
        │
        ▼
 Compress
        │
        ▼
 Encrypt
        │
        ▼
   Archive
```

Signed archive:

```text
File / Directory
        │
        ▼
 Compress
        │
        ▼
    Sign
        │
        ▼
   Archive
```

Signed and encrypted archive:

```text
File / Directory
        │
        ▼
 Compress
        │
        ▼
 Encrypt
        │
        ▼
    Sign
        │
        ▼
   Archive
```

---

# Current Limitations

Directory compression is still under active development.

The current implementation preserves file contents but does not yet rebuild all filesystem metadata such as symbolic links during extraction. This functionality is planned for a future revision.

---

# See Also

* [QtAI Overview](docs/qtai//qtai.md)
* [QSodium](docs/qtai/qsodium.md)
* [QML Zstd](docs/qtai/qml-zstd.md)
