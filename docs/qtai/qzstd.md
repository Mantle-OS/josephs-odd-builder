# QZstd

`QZstd` is the Qt/C++ layer over [`job_zstd`](../job/zstd_overview.md).

**This library does not implement any compression, encryption, or signing itself.** Every real operation happens in `job_zstd`, this library exists so Qt/QML code can drive that engine through `QString`/`QSecureMem`/`Q_PROPERTY` instead of `std::string`/`std::filesystem::path`/plain C++ getters.

If you are looking for a QML interface, see [qml-zstd.md](qml-zstd.md).

---

## Library split

| Library     | Purpose                                          |
| ----------- | ------------------------------------------------ |
| `qt-zstd`   | C++ / QtCore library documented here.            |
| `qml-zstd`  | QML-facing elements documented separately.       |

This document only covers the C++ side.

---

## Dependencies

`qt-zstd` links against:

* Qt6::Core
* `job_zstd` (all compression/crypto/signing logic lives here, see [zstd_overview.md](../job/zstd_overview.md))
* `job_crypto`, libsodium, `qt-sodium` (for `QSecureMem`, used to carry key material without ever passing through a `QString`)

---

## Two ownership patterns

Every class here relates to its `job::zstd::` counterpart one of two ways, and which one matters for how state actually flows.

* **Direct inheritance** — `QZstdCompressor : public job::zstd::JobZstdCompressor`, and the same for `QZstdDecompressor`, `QZstdCompressorCrypto`, `QZstdDecompressorCrypto`, `QZstdSign`. The Qt class *is* the real backend, blocking calls run directly on it, no pointer indirection. Each of these separately owns a `QZstdOptions*` purely as a QML-facing mirror of its own state, `input`/`output`/`compressionLevel`/etc, kept in sync by hand after every blocking call, `*m_opts = *this` followed by a manual `Q_EMIT m_opts->finished()`. That emit is deliberately manual, not wired through `setOnFinished()`, since these calls are synchronous, there is no background thread whose completion needs signalling, and wiring it through the callback caused a real ordering bug, the sync running one line after the emit rather than before it.
* **Composition** — `QZstd` alone. It inherits `QZstdOptions` for its own QML property surface, but holds `job::zstd::JobZstd*` by pointer rather than inheriting it, because `JobZstd` is the one class in the whole stack that genuinely runs on a background thread via `std::async`. Its `notifyFinished()` callback fires from that thread, so `QZstd` has to do a real cross-thread hop, `QMetaObject::invokeMethod(this, ..., Qt::QueuedConnection)`, to safely marshal state and the `finished()` emit back onto its own thread.

## Ownership tracking for externally-supplied options

Every class using the first pattern also supports `setOptions(QZstdOptions*)`, letting a caller (in practice, a QML wrapper that itself inherits `QZstdOptions`) hand over a `QZstdOptions*` it does not want copied, only shared. This is what `QmlCompressor` and friends do, they pass `this` down so their own inherited `QZstdOptions` base becomes the same object the backend reads and writes.

That introduces a real ownership question a plain pointer cannot answer on its own, was this `QZstdOptions*` allocated by the backend itself, in which case it must be deleted, or handed in from outside, in which case deleting it would destroy an object still in the middle of being torn down by its own owner, a genuine double-free/self-destruction hazard, not a hypothetical one. Every class using this pattern tracks a `bool m_ownsOpts`, `true` only when the class's own default constructor allocated the options object, `false` the moment `setOptions()` accepts an externally-supplied one. Every delete of `m_opts`, in `setOptions()` itself and in the destructor, is guarded by this flag, never by a null check alone, since a borrowed pointer is just as often non-null as an owned one.

## Options mirror

### QZstdOptions
Pure `QObject` property shell, no `job::zstd::` backend of its own.

* `input`, `output`, `compressionLevel`, `preserveEmptyDirectories`, `preserveSymlinks`, `recursiveDirectories` are read-write, matching `job::zstd::JobZstdOptions` field for field
* `current`, `total`, `errorString` are read-only from QML's side, they only ever change as a reflection of real backend state, never as user input
* `finished` signal, the one thing every wrapper class below actually fires

## Blocking pipeline

### QZstdCompressor / QZstdDecompressor
Thin, fully blocking wrappers over `job::zstd::JobZstdCompressor`/`JobZstdDecompressor`.

* `compress()` / `decompress()` call `execute()` directly and return its result, no threading, the calling thread blocks for the full duration
* both connect their owned `QZstdOptions`'s property-changed signals straight into the real backend's setters, so setting `compressor.input = "..."` from QML immediately reaches the C++ object doing the actual work

### QZstdCompressorCrypto / QZstdDecompressorCrypto
Same shape, adds a `QSecureMem` key.

* `setEncryptionKey()` / `setDecryptionKey()` take `QSecureMem` directly, never a string, matching `job::zstd::JobZstdCompressorCrypto`/`DecompressorCrypto`'s own in-memory key model
* `compressAndEncrypt()` / `decryptAndDecompress()` still sync and report through `m_opts` on a missing-key failure, not just on success, a caller checking `errorString()` after a `false` return gets a real explanation either way

### QZstdSign
Thin wrapper over `job::zstd::JobZstdSign`, file-path keys only, matching the underlying class exactly.

* `publicKeyFile()` / `setPublicKeyFile()`, `privateKeyFile()` / `setPrivateKeyFile()` read and write real files on disk
* deliberately has no in-memory key setter, that capability already exists elsewhere in the stack (`QSodiumKeys`/`QSecureMem`), duplicating it here would give the same keys two different, potentially inconsistent representations

## Async orchestration

### QZstd
The one class that composes rather than inherits, wrapping `job::zstd::JobZstd`'s `std::async`-based pipeline.

* `compress()` / `decompress()` run the plain pipeline; `compress(bool sign, bool encrypt)` / `decompress(bool verify, bool decrypt)` run the full crypto-and-signing pipeline in whichever combination is requested
* `publicKeyFile()` / `setPublicKeyFile()`, `privateKeyFile()` / `setPrivateKeyFile()` forward straight to the owned `job::zstd::JobZstd`'s own file-based signing key setters
* `getPrivateKey()` / `setPrivateKey(QSecureMem)` carries the symmetric encryption key, independent of the signing keypair
* `finished` fires only after a real cross-thread hop, state, `current`/`total`/`errorString`, is read from the background thread the instant the operation completes, then handed across via `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` before the signal emits on `QZstd`'s own thread