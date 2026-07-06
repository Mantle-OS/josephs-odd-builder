# QSodium

`QSodium` is a Qt/C++ library that provides a Qt-friendly interface for common cryptographic tasks: secure memory, key handling, password hashing, file signatures, hashing, random bytes, and symmetric encryption.

**This library does not implement any cryptography itself.** Every class here is a thin Qt-facing shim over [`job::crypto`](../job/crypto_overview.md) — QString/QByteArray/QDir conversions in, `job::crypto` types out. All real cryptographic work happens in `job_crypto`; this library exists so Qt/QML code never has to touch `std::string`/`std::vector<unsigned char>` directly.

QSodium is not AI-specific. It lives in the QtAI tree because QtAI needs these primitives for sessions, packages, signatures, encrypted data, and local configuration.

If you are looking for a QML interface, see [qml-sodium.md](qml-sodium.md).

---

## Library split

| Library      | Purpose                                          |
| ------------ | ------------------------------------------------ |
| `qt-sodium`  | C++ / QtCore library documented here.            |
| `qml-sodium` | QML-facing sodium helpers documented separately. |

This document only covers the C++ side.

---

## Dependencies

`qt-sodium` links against:

* Qt6::Core
* `job_crypto` (all cryptographic logic lives here — see [crypto_overview.md](../job/crypto_overview.md))
* libsodium directly

It does not require Qt Quick, Qt QML, Qt Widgets, or any AI runtime.

---

## Two binding patterns

Classes in this library bind to their `job::crypto` counterpart one of two ways:

* **Direct inheritance** — `QSecureMem : public job::crypto::JobSecureMem`, `QSodiumCryptoSign : public job::crypto::JobCryptoSign`. The Qt class *is a* `job::crypto` object with Qt-flavored methods added on top.
* **Owned-pointer composition** — `QSodiumKeys` holds a `job::crypto::JobCryptoKeys*` it allocates and frees itself. Used where the class isn't meant to be substitutable for its `job::crypto` counterpart.

`QSodium` itself holds its `job::crypto::JobCrypto` backend by value, since it's a `QObject` and can't inherit implementation from a non-`QObject` base the way `QSecureMem`/`QSodiumCryptoSign` do.

---

## QSodium

The `QObject` entry point and public API umbrella — `#include <qsodium.h>` pulls in every other class in this library.

* checks/reports libsodium initialization (via `job::crypto::JobCrypto`)
* `computeFileBlake2b()` — hex-string BLAKE2b of a file
* `encryptConfig()` / `decryptConfig()` — static convenience wrappers around `JobSecretBox`, converting `QByteArray` <-> `std::vector<unsigned char>` at the boundary

The lower-level classes below can also be used directly without going through `QSodium`.

---

## QSecureMem

`job::crypto::JobSecureMem`, inherited directly — an RAII buffer for sensitive byte data (secure allocation, zero-on-release, constant-time comparison), with Qt-flavored accessors added on top.

* `toBase64()` / `fromBase64()` — the normal path for moving key material to/from `QString`
* `toString()` / `fromBase64toString()` — pass straight through to the inherited `job::crypto::JobSecureMem` versions, which are gated behind `JOB_SECUREMEM_ALLOW_STRING` and `!NDEBUG` and throw otherwise. There is no separate Qt-level gating macro — the guard is entirely `job_crypto`'s.
* `appendTo(QByteArray*)` — appends the raw secure bytes onto an existing `QByteArray`

`QSecureMem` reduces accidental secret retention versus plain heap memory, but that protection ends the moment data is copied into a `QString`/`std::string`/ordinary `QByteArray` — those are regular application memory again, unprotected.

---

## QSodiumKeys

Wraps `job::crypto::JobCryptoKeys` (owned by pointer) — asymmetric keypair management.

* `KeyType` is a direct alias of `job::crypto::JobCryptoKeys::KeyType` (`Exchange` / X25519, `Sign` / Ed25519) — not a redeclared Qt enum
* `createKeys()` / `createSeedKeys()` — fresh or deterministic keypair generation
* `createKeysAndSave()` / `saveKeys()` / `loadKeysFromDisk()` — round-trip a keypair to/from disk, translating `QString`/`QDir` paths to `std::filesystem::path`
* `createClientSessionKeys()` / `createServerSessionKeys()` — X25519 session key derivation against a peer's public key
* Public keys are Base64 `QString`; private keys are `QSecureMem`

---

## QSodiumCryptoSign

`job::crypto::JobCryptoSign`, inherited directly — detached Ed25519 file signing and verification. Because `JobCryptoSign` itself extends `JobCryptoKeys`, this class also gets key management (`loadKeys()`, `setPublicKey()`, `addKeyDirectory()`) for free through the inheritance chain, not by re-wrapping `QSodiumKeys`.

* `signFile()` / `verifyFile()` — files are streamed in chunks, never loaded whole into memory
* `signAssociatedFile()` — signs a file already associated with an open handle
* `pubKey()` — `QString` accessor (named distinctly from the inherited `publicKey()`, which returns `std::string`)

This signs the bytes on disk as-is. It does not canonicalize JSON, YAML, line endings, or metadata before signing — the caller is responsible for signing a stable, canonical representation if that matters for their use case.

---

## QSodiumPasswordUtils

Static-only wrapper over `job::crypto::JobPasswordUtils` — Argon2id password handling.

* `hashPasswordForStorage()` / `verifyPasswordAgainstStorage()` — password hashing for storage and verification
* `deriveKeyFromPassword()` — derives a `QSecureMem` key from a password + salt, sized for `QSodiumSecretBox`

Do not use a stored password hash as an encryption key — password verification and key derivation are different workflows with different security properties, even though both start from the same password.

---

## QSodiumSecretBox

Static-only wrapper over `job::crypto::JobSecretBox` — symmetric authenticated encryption.

* `generateNonce()` — fresh random nonce per call
* `encrypt()` / `decrypt()` — key is `QSecureMem` in and out; plaintext/ciphertext are ordinary `QByteArray`

As with `job::crypto::JobSecretBox`, nonce discipline (never reuse a nonce with the same key) is the caller's responsibility — this class does not track or enforce uniqueness. Because plaintext here travels as `QByteArray`, not `QSecureMem`, callers should avoid keeping sensitive plaintext in it any longer than necessary.

---

## QSodiumHash

Static-only wrapper over `job::crypto::JobHash` — BLAKE2b hashing via libsodium's generic hash API.

* `hashBuffer()` / `hashFile()` — both support an optional keyed-MAC mode via an optional `QByteArray` key
* used for integrity checks, package verification, cache validation, manifest data

---

## QExtraRandom

Static-only wrapper over `job::crypto::JobRandom` — every method is a direct one-line delegation (secure bytes, uniform/normal/bernoulli distributions, global seed control), with `randomSalt()` converting the result to `QByteArray`.

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
QSodiumKeys / QSodiumCryptoSign
  -> signature
  -> verify later
```

File integrity:

```text
file
  -> QSodiumHash
  -> digest
```