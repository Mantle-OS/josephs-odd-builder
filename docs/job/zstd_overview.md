# Job Zstd

Compression, encryption, and signing for job archives.

job_zstd links against `job_core` (SplitMix64, for deterministic benchmark payloads only), `job_crypto`, and zstd. It has no Qt dependency.

- `job::zstd` provides self-describing compressed archives, an optional encryption envelope, and detached signing
- every archive, whether it holds a single file, a directory tree, or nothing at all, starts with a magic tag identifying exactly what follows, so decompression never has to guess

## Foundational

### JobZstdOptions
Shared state base class inherited by every stage below (paths, progress, compression level, error string, structural flags).

- `magicFileString()` / `magicDirString()` / `magicEmptyDirString()` / `magicLinkString()` identify what an archive (or an entry inside one) contains
- `preserveEmptyDirectories()`, `preserveSymlinks()`, `recursiveDirectories()` control structural behavior on both compress and decompress
- `setOnFinished()` / `notifyFinished()` give subclasses a completion callback with no other observer machinery attached

### JobZstdIO
A `std::streambuf` wrapping the raw zstd streaming API (`ZSTD_CStream`/`ZSTD_DStream`).

- `xsputn()` / `xsgetn()` overrides move bulk data directly, bypassing the default one-byte-at-a-time streambuf path entirely
- distinguishes a clean end of stream from truncation (`wasTruncated()`) and from a corrupted frame (`hadDecodeError()`) as three separate states
- because it only depends on `std::streambuf`, anything else that also speaks streambuf (a real file, an in-memory buffer, an encrypting transport) can sit underneath it with zero coupling

### job::zstd::utils (job_zstd_wire.h)
Minimal length-prefixed binary framing (`writeString`/`readString`, `writeU64`/`readU64`, `writeU8`/`readU8`) used for every archive header and entry tag. `readString()` refuses a length prefix past a fixed ceiling before ever allocating, so a corrupt or hostile length can't trigger a runaway allocation.

### job_zstd_entry.h
Free functions shared by the compressor and decompressor.

- `collectEntries()` walks a directory tree, classifying each child as a file, a directory (has children), an empty directory, or a symlink, with cycle detection (`JobZstdDirGuard`) for symlinked directories that loop back on themselves
- `safeJoin()` rejects any archive-supplied relative path that would resolve outside a given base directory
- `verifyNoSymlinkComponents()` walks every component of an extraction target from the filesystem root down, refusing if anything already on disk along that path is a symlink

## Compression and decompression

### JobZstdCompressor / JobZstdDecompressor
The plain, unencrypted pipeline. Compression walks the input with `collectEntries()`, writes a magic tag followed by each entry's own tag and payload, all through a `JobZstdIO` writing to the real output file. Decompression reads the top level tag first and dispatches to the matching handler, whether that means restoring a flat file, a full directory tree, a single empty directory, or a single symlink.

### JobZstdCompressorCrypto / JobZstdDecompressorCrypto
The encrypted pipeline. The archive content itself is identical to the plain pipeline, encryption is an outer envelope built from `JobZstdEncryptingTransport`/`JobZstdDecryptingTransport`, so `JobZstdIO` never has any awareness that encryption exists at all.

- compression happens before encryption, so the compressor still gets a real compression ratio rather than trying to compress already-encrypted, high-entropy bytes
- `hasKeys()` on both classes fails fast with a clear error if a correctly-sized key has not been set, rather than letting an absent key surface later as a confusing failure inside the transport

### JobZstdEncryptingTransport / JobZstdDecryptingTransport
`std::streambuf` implementations that sit between `JobZstdIO` and the real file, encrypting or decrypting fixed-size chunks of already-compressed bytes as they pass through (via `job::crypto::JobSecretBox`).

- each chunk gets its own fresh nonce and is framed with the same length-prefixed wire format used elsewhere in the library
- a failed authentication check is reported as a distinct state (`hadAuthenticationError()`) from a merely truncated stream (`wasTruncated()`), since a caller should react differently to a cut-off download than to a tampered or wrong-key archive

## Signing

### JobZstdSign
Detached Ed25519 signing and verification of an archive file, built on `job::crypto::JobCryptoSign`.

- `setPublicKeyFile()` / `setPrivateKeyFile()` always re-read from disk rather than trusting a cached path match, since a key file can be rotated in place at the same path
- `setPrivateKeyFile()` verifies the loaded private key actually pairs with the current public key, by re-deriving a public key from the private key's seed and comparing, rather than trusting the key's own embedded copy of its public half
- `signFile()` / `verifyFile()` operate on explicit input and output paths rather than through `JobZstdOptions`'s shared input/output, since signing genuinely involves two independent files (the archive, and wherever the signature lands)

## Orchestration

### JobZstd
Async wrapper over the five classes above, using `std::async`/`std::future` rather than a thread pool.

- `compress()` / `decompress()` run the plain pipeline; `compress(bool sign, bool encrypt)` / `decompress(bool verify, bool decrypt)` run the full crypto and signing pipeline in whichever combination is requested
- the finished callback (inherited from `JobZstdOptions`) fires from the background thread, not the calling thread. There is no Qt-style queued connection marshalling a caller back to its own thread
- there is no live progress relay across the async boundary. `current()`/`total()` are only meaningful to read once an operation has actually completed