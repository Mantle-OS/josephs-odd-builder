# Job Crypto

The cryptographic foundation of job.

jobcrypto links against libsodium only — no other `job::*` library dependencies.

- `job::crypto` wraps libsodium init, secure memory, keys/signing, hashing, symmetric encryption, and password handling

## Foundational

### JobCryptoInit
One-time libsodium init flag.
- `sodium_init()` happens once
- used by every other class in the library on first real use (lazy init)

### JobSecureMem
Secure buffer wrapper using libsodium.
- allocation uses `sodium_malloc`
- memory is locked + zeroed on allocate
- compares use constant-time compare (`sodium_memcmp`)
- supports clearing and free (zero then free)
- copy/move/assignment all preserve secure-zero-on-replace semantics

String conversion:
- `toBase64()` / `fromBase64()` are the normal path for moving key material to/from text
- `fromBase64()` scrubs its own intermediate decode buffer before returning
- `toString()` / `fromBase64toString()` are an internal-only debug escape hatch — gated behind `JOB_SECUREMEM_ALLOW_STRING` and `!NDEBUG`, disabled by default, and throw in production builds. Discouraged outside of local debugging; never intended for production call sites.

Used as the "don't leave plaintext in heap" type throughout the stack (ex: `job::net::JobUrl` password storage, `job::zstd`'s crypto pipeline).

### JobRandom
Random utilities split into two lanes.
- secure bytes (libsodium `randombytes_*`)
- per-thread PRNG (`std::mt19937_64`) seeded from secure bytes (or from a global seed)
- includes basic distributions (uniform / normal / bernoulli)
- global seed mode exists for repeatable runs (thread-local engines still get derived seeds)

## Keys and identity

### JobCryptoKeys
Base keypair container — public key (base64 text) + private key (`JobSecureMem`).
- `KeyType::Exchange` (X25519) or `KeyType::Sign` (Ed25519)
- `createKeys()` / `createSeedKeys()` generate a fresh or deterministic keypair
- `createClientSessionKeys()` / `createServerSessionKeys()` derive X25519 session keys against a peer's public key
- `validPublicKey(path)` checks a key file decodes to the right byte length for its type — structural validation only, not a trust judgment
- `publicKeyData(path)` reads and normalizes a public key file's base64 content
- `privateKeyMatchesPublicKey(type)` re-derives a public key from a private key's actual seed (not the embedded copy) and compares — proves the pairing is mathematically real, not just structurally plausible
- `saveKeys()` / `loadKeysFromDisk()` round-trip a keypair to/from a directory

### JobCryptoSign
Detached Ed25519 signing, built on `JobCryptoKeys`.
- `signFile()` / `verifyFile()` stream a file through `crypto_sign_init/update/final_*` rather than loading it whole
- `verifyFile()` overloads accept either base64 text or raw signature bytes
- `signAssociatedFile()` / `verifyAssociatedFile()` reuse an already-open file handle, restoring its read position afterward

## Hashing, encryption, and passwords

### JobHash
BLAKE2b hashing via `crypto_generichash`.
- `hashBuffer()` / `hashFile()`, both support an optional keyed-MAC mode
- `hashFile()` streams in 1MB chunks — sized for large files (models, textures, archive payloads)

### JobSecretBox
Symmetric authenticated encryption (`crypto_secretbox_easy` — XSalsa20-Poly1305).
- `encrypt()` / `decrypt()` operate on `JobSecureMem` keys and plain byte buffers
- `generateNonce()` produces a fresh 24-byte random nonce per call via `JobRandom`

**Nonce discipline is entirely the caller's responsibility.** Reusing a nonce with the same key breaks confidentiality (XOR of the two plaintexts leaks) and can enable forgery — this is a hard rule of the underlying construction, not a `JobSecretBox`-specific quirk. This class does not track or enforce nonce uniqueness itself; it trusts `generateNonce()`'s randomness (24 bytes is large enough that random generation is the accepted safe practice here, unlike shorter-nonce schemes that require a counter). A caller who supplies their own nonce instead of using `generateNonce()`, or who persists/reuses a nonce across calls with the same key, is responsible for that guarantee themselves.

One-shot API — chunking a larger stream into multiple encrypt/decrypt calls (each with its own fresh nonce) is the caller's responsibility (see `job::zstd`'s encrypting/decrypting transports for a worked example).

### JobPasswordUtils
Argon2id password handling.
- `hashPasswordForStorage()` / `verifyPasswordAgainstStorage()` — libsodium's self-contained password hash format (`crypto_pwhash_str`)
- `deriveKeyFromPassword()` — raw key derivation (`crypto_pwhash`) for turning a password + salt into a `JobSecureMem` key, e.g. for `JobSecretBox`

## Utilities

### job::crypto::utils
Free functions for base64 <-> binary conversion (`base64ToBin`, `toBase64`), used internally throughout the library and available for direct use.

### JobCrypto
Small convenience facade over the above.
- `computeFileBlake2bHex()` — hex-string wrapper over `JobHash`
- `encryptConfig()` / `decryptConfig()` — static wrappers over `JobSecretBox`