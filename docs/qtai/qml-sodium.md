# QML Sodium

`qml-sodium` provides a QML interface to the `qt-sodium` library.

It exposes common cryptographic workflows as QML elements so they can be used directly from Qt Quick applications. The underlying cryptographic operations are still performed by the C++ `qt-sodium` library.

If your application is written primarily in C++, or you need lower-level control over cryptographic operations, see [qsodium.md](qsodium.md).

---

## Design

The goal of `qml-sodium` is to make common cryptographic tasks available to QML without requiring applications to write their own wrapper layer.

Rather than exposing every C++ class directly, the library presents a collection of QML elements that represent common workflows. Each element owns a small amount of bindable state through QML properties and performs work through `Q_INVOKABLE` methods. Results are written back into the element's properties, allowing normal QML bindings to update automatically.

This keeps the QML API declarative while leaving the lower-level cryptographic implementation in the C++ library.

---

## Dependencies

`qml-sodium` depends on:

* Qt Core
* Qt Quick
* Qt Quick Controls
* Qt QML
* `qt-sodium`
* `qt-ai-utils`

The library does not implement cryptographic primitives itself. All cryptographic operations are delegated to `qt-sodium`.

---

## Library Layout

The library currently provides the following QML types.

### QmlSodiumBox

`QmlSodiumBox` exposes password-derived symmetric encryption to QML.

It manages the QML-facing state required for encryption and decryption, including:

* Password
* Salt
* Cipher text
* Nonce

It provides invokable methods for:

* Encrypting a string.
* Decrypting a string.
* Generating a new salt.

---

### QmlSodiumCryptoSign

`QmlSodiumCryptoSign` exposes detached file signing and signature verification.

It provides QML properties for:

* File path
* Public key
* Detached signature

Functions are provided for:

* Signing a file.
* Signing an associated file.
* Verifying a detached signature.
* Computing a BLAKE2b file hash.
* Loading a public key from disk.

---

### QmlSodiumHash

`QmlSodiumHash` exposes hashing functionality to QML.

Unlike the other QML types, `QmlSodiumHash` is registered as a singleton and is intended to be called directly from QML.

It supports:

* Hashing text buffers.
* Hashing files.
* Tracking the most recently calculated hash.

---

### QmlSodiumKeys

`QmlSodiumKeys` exposes key management to QML.

It provides support for:

* Creating key pairs.
* Loading key pairs.
* Saving key pairs.
* Validating the current key state.

Supported key types are:

* Exchange
* Sign

---

### QmlSodiumPasswordUtils

`QmlSodiumPasswordUtils` provides password-related helper functions for QML.

It supports:

* Password hashing for storage.
* Password verification.

This type is intended for login dialogs, account creation, and other user-facing authentication workflows.

---

### SecureTextField

`SecureTextField` is a reusable QML control intended for password and secret entry.

It wraps a standard `TextField`, emits the entered value through `valueCommitted()`, and can automatically clear the field when editing has completed or focus is lost.

It exists to simplify password entry throughout QtAI applications.

---

## Notes

`qml-sodium` is a QML interface to `qt-sodium`, not a separate cryptographic implementation.

Sensitive operations are performed by the underlying C++ library. QML values remain ordinary QML strings and properties, so applications handling highly sensitive data should continue to use the lower-level C++ interfaces where appropriate.

---

## See Also

* [QtAI Overview](../qtai.md)
* [QSodium](qsodium.md)
