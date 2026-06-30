# QSodium

`QSodium` is a Qt/C++ library built on top of libsodium.

It provides a QtCore-only interface for common cryptographic tasks such as secure memory, key handling, password hashing, file signatures, hashing, random bytes, and symmetric encryption.

QSodium is not AI-specific. It lives in the QtAI tree because QtAI needs these primitives for sessions, packages, signatures, encrypted data, and local configuration. The library can also be used by other Qt projects that need a small Qt-friendly wrapper around libsodium.

If you are looking for a QML interface, see [qml-sodium.md](qml-sodium.md).

---

## Library split

Sodium support is split into two libraries:

| Library                | Purpose                                          |
| ---------------------- | ------------------------------------------------ |
| `qt-sodium`            | C++ / QtCore library documented here.            |
| `qtdeclarative-sodium` | QML-facing sodium helpers documented separately. |

This document only covers the C++ side.

---

## Dependencies

`qt-sodium` depends on:

* Qt Core
* libsodium

It does not require Qt Quick, Qt QML, Qt Widgets, or any AI runtime.

---

## QSodium

`QSodium` is the QObject entry point for the C++ library.

It checks whether libsodium is initialized and initializes it if needed. It also exposes a small set of convenience functions for common operations:

* Verify a detached file signature.
* Compute a BLAKE2b file hash.
* Encrypt a small payload.
* Decrypt a small payload.

The lower-level classes can also be used directly.

---

## QSecureMem

`QSecureMem` is an RAII buffer for sensitive byte data.

It uses libsodium secure allocation, clears memory before release, and supports constant-time comparison.

Typical uses include:

* Private keys
* Derived keys
* Decrypted payloads
* Short-lived secret material

`QSecureMem` helps reduce accidental secret retention in normal heap memory, but it does not make every copy safe. Once data is converted to `QString`, `std::string`, or an ordinary `QByteArray`, it is regular application memory again.

String conversion is gated by `QSECUREMEM_ALLOW_STRING`.

---

## QSodiumKeys

`QSodiumKeys` manages asymmetric key material.

It supports:

* Ed25519 signing keys
* X25519 key exchange keys
* Random key generation
* Seed-based key generation
* Client/server session key derivation

Public keys are stored as Base64 `QString` values. Private keys are stored in `QSecureMem`.

---

## QSodiumCryptoSign

`QSodiumCryptoSign` builds on `QSodiumKeys` and provides detached file signing and verification.

Files are read in chunks instead of being loaded completely into memory.

This is useful for:

* Package files
* Manifests
* Compressed archives
* Cached files
* Any file that needs a detached signature

This class signs the bytes on disk. It does not canonicalize JSON, YAML, line endings, or metadata before signing.

---

## QSodiumPasswordUtils

`QSodiumPasswordUtils` provides password hashing, password verification, and password-derived keys.

It supports two separate jobs:

* Hash a password for storage.
* Derive a symmetric key from a password and salt.

The derived key is written to `QSecureMem` and is sized for SecretBox usage.

Do not use a stored password hash as an encryption key. Password verification and key derivation are different workflows.

---

## QSodiumSecretBox

`QSodiumSecretBox` provides symmetric authenticated encryption.

It can:

* Generate a nonce.
* Encrypt a payload.
* Decrypt and authenticate a payload.

The key is supplied as `QSecureMem`. Decrypted plaintext is returned in `QSecureMem`.

The input plaintext and ciphertext are ordinary `QByteArray` values, so callers should avoid keeping sensitive plaintext around longer than needed.

---

## QSodiumHash

`QSodiumHash` provides BLAKE2b hashing through libsodium's generic hash API.

It supports:

* Hashing a memory buffer.
* Hashing a file.
* Streaming file hashing in fixed-size chunks.
* Configurable digest size within libsodium limits.

This is used for integrity checks, package verification, cache validation, and manifest data.

---

## QExtraRandom

`QExtraRandom` provides random byte helpers used by the rest of the library.

It is used for salts, nonces, and other random data needed by sodium workflows.

---

## Common workflows

Password-derived encryption:

```text
password
  -> QSodiumPasswordUtils
  -> QSecureMem key
  -> QSodiumSecretBox
```

Detached file signature:

```text
QSodiumKeys
  -> QSodiumCryptoSign
  -> signature
  -> verify later
```

File integrity:

```text
file
  -> QSodiumHash
  -> digest
```

---

## Notes

QSodium is intentionally small. It does not try to wrap every libsodium API.

The current goal is to provide the pieces needed by QtAI libraries and applications without pulling crypto code into each library separately.
