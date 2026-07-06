# QML Zstd

`qml-zstd` is the QML-facing layer over [`qt-zstd`](qzstd.md).

This document assumes familiarity with [qzstd.md](qzstd.md), every class here wraps a `qt-zstd` counterpart directly, most by inheriting it, exposing the same state QML already needs to bind to as `Q_PROPERTY`/`Q_INVOKABLE` surface.

---

## Dependencies

`qml-zstd` links against:

* Qt6::Core, Qt6::Gui, Qt6::Quick, Qt6::QuickControls2, Qt6::Qml
* `qt-ai-utils` (property/pointer macros)
* libsodium, `job_crypto`, `qt-sodium`, `qml-sodium`, `qml-sodiumplugin` (for `QSecureMem`, `QmlSecureMem`, `QmlSecureMemInput`, `QmlSodiumKeys`, all reused rather than re-implemented)
* `job_zstd`, `qt-zstd`

---

## The QZstdOptions bridge

Every wrapper class below that holds a `qt-zstd` backend by inheritance (`QmlCompressor`, `QmlCyptoCompressor`, `QmlDecompressor`, `QmlCryptoDecompressor`) also inherits `QZstdOptions` itself, rather than exposing the backend's options through a separate accessor. This is deliberate, `QZstdCompressor` (and its siblings) is-a `job::zstd::JobZstdCompressor`, so the QML class inheriting `QZstdOptions` directly means `input`, `output`, `compressionLevel`, and the three structural flags are already real, bindable properties with no extra plumbing.

`QmlZstd` is the one exception, it wraps `job::zstd::JobZstd` by composition rather than inheritance (see [qzstd.md](qzstd.md) for why), but it also inherits `QZstdOptions` for the same reason, giving QML the same property surface regardless of which underlying pattern the class uses.

---

## Blocking wrappers

### QmlCompressor
`QML_ELEMENT` wrapping `QZstdCompressor`, plain, unencrypted, fully blocking.

* `compress()` — calls the real `execute()` directly, freezes the UI thread for the duration, on purpose, this element exists to demonstrate the plain blocking API without async machinery involved
* inherits every `QZstdOptions` property directly, `myCompressor.compressionLevel = 9` reaches the real backend immediately

### QmlDecompressor
Same shape, wrapping `QZstdDecompressor`.

* `decompress()` — same blocking contract as `QmlCompressor::compress()`

### QmlCyptoCompressor
Wraps `QZstdCompressorCrypto`, adds password-derived symmetric encryption on top of the plain compressor's shape.

* `password` (`QmlSecureMem*`, read-only property) — the caller's password lives here, copied in via `setPassword(QmlSecureMem*)`, never as a `QString`, matching `QmlSecureMemInput`'s own guarantee that a typed secret never exists as text
* `salt` (`QString`, read-write) — auto-generated on first use if left empty, exposed so a caller can display and persist it, decrypting the same archive later requires the exact same salt, the same password with a different salt derives a completely different key
* `compress(bool autoSalt = false)` — decides the salt first, generates a fresh one if `autoSalt` is true or none exists yet, then derives the key and only then calls `compressAndEncrypt()`. Deriving before deciding the salt was an early, real bug here, the salt actually used to derive the key and the salt left visible in the property could disagree if the order were reversed

### QmlCryptoDecompressor
Wraps `QZstdDecompressorCrypto`, mirrors `QmlCyptoCompressor`'s password/salt shape exactly.

* `password`, `setPassword(QmlSecureMem*)` — identical pattern to the compressor side
* `salt` (`QString`, read-write) — must be set explicitly to the exact value the encrypt side produced, there is no auto-generate fallback here. A missing salt on decrypt means the wrong archive or a skipped step, silently generating a fresh one would just derive the wrong key and fail with no clear signal why
* `decompress()` — derives the key from `password` + `salt`, refuses outright if either is missing, then calls `decryptAndDecompress()`

---

## Async orchestration

### QmlZstd
`QML_SINGLETON` wrapping `QZstd`.

* `compress()` / `decompress()`, `compress(bool sign, bool encrypt)` / `decompress(bool verify, bool decrypt)` — same signatures as the underlying `QZstd`, all genuinely asynchronous, the calling thread is never blocked
* `signingKeys` (`QmlSodiumKeys*`, `CONSTANT`) — an owned `QmlSodiumKeys` instance handles Ed25519 keypair generation and disk round-tripping, its `publicKeyFileChanged`/`privateKeyFileChanged` signals are connected internally to push those paths straight into the real `QZstd` backend, rejections are logged via `qWarning()` rather than silently discarded, since a `[[nodiscard]] bool` setter failing quietly here would otherwise surface much later as a confusing compress/decompress error with no obvious connection to the key that never actually loaded
* there is no in-memory signing key path on this class, only file paths, matching `QZstdSign`'s own scope exactly, `QSodiumKeys`/`QSecureMem` already exist for in-memory key handling elsewhere in the stack, duplicating that here was deliberately avoided
* `finished` fires only after `QZstd`'s own cross-thread hop completes, safe to connect to directly from QML without additional marshalling on the QML side

---

## ZStdOptionsPopup

A reusable `Popup`, shipped as a `QML_FILES` entry in this module rather than living in any single application, since every wrapper class above shares the exact same `QZstdOptions` surface.

* `property QtObject target` — bind to any `QZstdOptions`-inheriting object, `QmlZstd`, `QmlCompressor`, or any of its siblings
* every control inside writes straight through to `target` on every change, there is no separate commit/cancel step, closing the popup does not discard anything, it was never staged in the first place
* edits compression level and the three structural flags (`preserveEmptyDirectories`, `preserveSymlinks`, `recursiveDirectories`), and displays `errorString` read-only
* deliberately does not bake in a compress-vs-decompress assumption about `input`/`output`, different pages want different picker types for those two fields, a single `FolderDialog` for compression's output, a derived path for decompression's, so path selection is left to each page's own pickers rather than guessed inside the shared component
* size and position are not fixed by the component itself, a typical usage sets `width`/`height` as a fraction of the parent and centers explicitly, `anchors.centerIn: parent` alongside explicit `width`/`height`, since a `Popup` with no declared position defaults to its parent's top-left corner, not the window's center

---

## Example application

The bundled example app demonstrates all four operating modes side by side, plain blocking compress, plain blocking decompress, the full async pipeline including optional sign/encrypt, and password-based encrypt/decrypt built entirely from `QmlCyptoCompressor`/`QmlCryptoDecompressor`. Each page states plainly what it is showing and why, the blocking pages exist specifically to demonstrate the plain API without async machinery, the async page exists to show the version a real application would actually use.