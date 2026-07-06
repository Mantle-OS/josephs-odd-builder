# QML Sodium

`qml-sodium` is the QML-facing layer over [`qt-sodium`](qsodium.md). It provides QML types for secure text input, password-derived encryption, hashing, key management, and file signing — all built so sensitive material stays in `QSecureMem`/`QmlSecureMem` rather than passing through `QString` wherever avoidable.

This document assumes familiarity with [qsodium.md](qsodium.md) — every class here wraps a `qt-sodium` counterpart.

---

## Dependencies

`qml-sodium` links against:

* Qt6::Core, Qt6::Gui, Qt6::Quick, Qt6::Qml
* libsodium directly
* `job_crypto`
* `qt-sodium`
* `qt-ai-utils` (property/pointer declaration macros used throughout — `QP_RW`, `QP_RO`, `QP_PTR_RO`)

---

## Secure input and storage

### QmlSecureMemInput — the highlight of this library

A hand-built `QQuickItem` password field. It does not use `TextInput`, and it does not hold the typed password as a `QString` at any point — not even internally, not even transiently.

Keystrokes go directly from `QKeyEvent::text()`'s raw UTF-8 bytes into a fixed 64-byte `QSecureMem` buffer, reached through the item's `memory` property (a `QmlSecureMem*`). This matters because Qt's own `TextInput` with `echoMode: Password` only masks the *display* — its backing model is a real `QString` holding the actual plaintext in ordinary heap memory. This field avoids that gap by construction: the plaintext string is never assembled in the first place.

Two details worth understanding, not just using:

* **Byte-accurate backspace.** UTF-8 characters aren't fixed-width, so a naive "remove one byte per backspace" would corrupt multi-byte characters. `QmlSecureMemInput` tracks the exact byte length of each character as it's typed (`m_inputByteLengths`), so backspace unwinds precisely that many bytes and zeroes exactly that freed range in secure memory — no stale plaintext left sitting in the buffer's tail.
* **Content-blind rendering.** The masking dots are pure count-based geometry (`m_maskCount`), rendered via raw `QSGGeometryNode`s. They carry zero information about what was typed beyond how many characters exist — there's no code path where the actual bytes ever reach the render tree.

`secureWipe()` clears and immediately re-arms the buffer (fresh 64-byte allocation) — call this explicitly if a form needs to discard a password without destroying the item, e.g. on a cancel action.

`length` (the QML-visible property) is a character count (`m_maskCount`), not a byte count — deliberately, since byte count could hint at character composition (e.g. presence of multi-byte characters) in a way a plain count doesn't.

**Example — capturing a password and handing it to `QmlSodiumPasswordUtils`:**

```qml
Item {
    QmlSodiumPasswordUtils { id: passUtils }
    QmlSecureMemInput {
        id: passUtilsField
        onReturnPressed: {
            if (passUtils.setPassword(passUtilsField.memory)) {
                passUtilsField.secureWipe()
            } else {
                console.error("Secure password memory copy failed.")
            }
        }
    }
}
```

Note the ownership handoff here: `setPassword()` *copies* the bytes out of `passUtilsField.memory` into `passUtils`'s own internal buffer — it doesn't take ownership of the input field's memory. That's why it's safe to call `secureWipe()` on the input field immediately afterward; the copy has already happened, and the field is free to discard its own contents.


### QmlSecureMem
Thin QObject handle around an owned QSecureMem*, sized at a fixed 64 bytes. This is the type QML code passes by pointer between components (QmlSodiumBox::setPassword(QmlSecureMem *source), QmlSecureMemInput.memory, etc.) rather than ever copying secret content through a QString-typed property.

* internalBuffer() — raw access to the underlying QSecureMem*, for classes that need to reach in and copyFrom()/clear() directly
* copyFromSecureMem() — allocates to match a source buffer's size and deep-copies its contents

Every consumer of this class follows the same pattern: copy bytes in via copyFrom, never assign a QString derived from it



---

## Password-derived encryption

### QmlSodiumBox
A self-contained "encrypt a string with a password" component — password (`QmlSecureMem`), salt/ciphertext/nonce (base64 `QString` properties, suitable for storage or transport).

* `generateNewSalt()` — fresh random salt via `QExtraRandom`
* `deriveKey()` — internal helper; derives the symmetric key from the current password + salt via `QSodiumPasswordUtils`, deriving a fresh salt first if none is set
* `encryptString()` / `decryptToString()` — the public workflow: derive key, then `QSodium::encryptConfig()`/`decryptConfig()`

Decrypted output from `decryptToString()` does pass through a `QString` at the return boundary — appropriate here since the caller explicitly asked for a `QString` back; the design intent is to keep the password and derived key out of `QString`, not the final decrypted result.

### QmlSodiumPasswordUtils
Narrower sibling of `QmlSodiumBox`, focused purely on password hashing rather than encryption.

* `password` — a `QmlSecureMem` slot, set via `setPassword(QmlSecureMem *source)` (copies bytes in, same pattern as everywhere else in this library)
* `hashForStorage()` / `verifyAgainstStorage()` — thin wrappers over `QSodiumPasswordUtils`
* `clearPassword()` — explicit wipe without destroying the component

---

## Signing and keys

### QmlSodiumCryptoSign
Wraps `QSodiumCryptoSign` (owned by pointer). `publicKey`/`privateKey` are file path properties — setting either one triggers an attempt to load both keys from disk once both files exist, via connected property-changed signals.

* `signFile()` / `signAssociatedFile()` / `verifyAssociatedFile()` — mirror the underlying `QSodiumCryptoSign` methods, syncing the resulting signature into the `signatureBase64` property
* `hasKeys()` — gates every signing/verifying call; delegates to the underlying `JobCryptoKeys::isValid()`
* `computeFileBlake2b()` — doesn't require keys at all; a plain hash convenience exposed alongside signing since both operate on `filePath`

### QmlSodiumKeys
Wraps `QSodiumKeys` (owned by pointer) — keypair generation and disk round-tripping for QML.

* `KeyType` is a separately declared `Q_ENUM`, not a direct reflection of `job::crypto::JobCryptoKeys::KeyType` — QML can't see a plain C++ `enum class` from a non-`QObject` type, so this mirrors it with matching explicit numeric values. The two enums must be kept aligned by hand if either ever changes.
* `create()` — generates a keypair and syncs `publicKeyBase64` from the result
* `saveKeysToDisk()` / `loadKeysFromDisk()` — resolve `keyDir` + `publicKeyFile`/`privateKeyFile` into full paths before delegating
* Setting `publicKeyBase64` directly also pushes that value into the underlying `QSodiumKeys` object via a connected signal — useful for wiring in an externally-supplied public key without generating one locally

---

## Hashing

### QmlSodiumHash
`QML_SINGLETON` wrapper over `QSodiumHash`.

* `hashBuffer(QString)` / `hashFile()` — both return a hex string and record it in `lastHash`
* `hashFile()` reads its target from the `filePath` property rather than taking a parameter

---

## Common workflow: password field to encrypted string

```text
QmlSecureMemInput (captures password, never as QString)
  -> QmlSodiumBox.setPassword(secureBuffer)
  -> QmlSodiumBox.encryptString(plainText)
  -> cipherText / nonce / salt (QString properties, safe to persist)
```