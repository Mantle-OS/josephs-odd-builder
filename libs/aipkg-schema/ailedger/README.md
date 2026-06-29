# `aipkg_ledger` — Cryptographic State Ledger Schemas

This directory contains the foundational data schemas that drive the **cryptographically verifiable distribution ledger** for the `QtAI` package management eco-system. These schemas are parsed by `job_msg_gen` to emit zero-allocation, performance-optimized, binary-serialized C++ data structures backed by MessagePack.

Unlike standard package repositories which rely on mutable flat indices, this system treats the package manifest registry as an **append-only, Merkle-tree-backed verifiable timeline**. This structure ensures absolute chain-of-custody, transparent authorship tracking, and immune protection against downstream file-swapping or data tampering.

---

## Core Architectural Layout

The files are structured into three logical operational tiers:

### 1. Foundational Primitives

* **`ledger_hash32.yaml`** (`AiPkgHash32`): High-density 32-byte cryptographic container mapping directly to streaming **BLAKE2b** hardware-aligned byte arrays.
* **`ledger_tx.yaml`** (`AiPkgTx`): Standard transaction boundary format validating sender/receiver public identities, ordering nonces, and `sig_from` verification states.

### 2. Merkle Tree & Log Integrity Proofs

* **`ledger_sth.yaml`** (`AiPkgSTH`): **Signed Tree Head**. Captures a high-performance snapshot of the verified Merkle Tree root hash and log size, bypassing the need to replay the timeline from genesis.
* **`ledger_audit_node.yaml`** (`AiPkgAuditNode`): Individual branch steps inside a validation proof, tracking sibling hashes and orientation.
* **`ledger_audit_path.yaml`** (`AiPkgAuditPath`): Collection arrays of sibling nodes used by the client to prove manifest membership against an active root.
* **`ledger_consistency.yaml`** (`AiPkgConsistencyProof`): Evaluates historical sub-tree states to mathematically guarantee that the append-only timeline has not been retroactively modified or re-ordered.
* **`ledger_block.yaml`** (`AiPkgBlock`): Linear timeline blocks that package transactions, bind them to the preceding block hash digest, and embed the current `STH`.

### 3. Supply-Chain Trust & Identity Management

* **`ledger_attestation.yaml`** (`AiPkgAttestation`): Independent cryptographic review logs certifying a specific manifest's version hash by an authorized testing authority.
* **`ledger_devlink.yaml`** (`AiPkgDevLink`): Maps public key identities directly to upstream host platforms (e.g., Hugging Face, GitHub) and records cryptographically verified ownership proofs.
* **`ledger_devsig.yaml`** (`AiPkgDevSig`): Detached multi-signature container allowing multiple independent developers to endorse or verify the same manifest content hash.
* **`ledger_delegate.yaml`** (`AiPkgDelegate`): Implements time-scoped authority sharing, allowing developers to safely pass package signing rights to automated CI build-runners without exposing master identities.
* **`ledger_revoke.yaml`** (`AiPkgRevoke`): The emergency break. Handles explicit, timeline-wide invalidation of compromised identity keys, compromised attestations, or broken/malicious package ranges.

---

## Compilation and Binding Integration

These files are compiled using the custom CMake automation macro `job_msgpack_gen_lib`. Code generation runs entirely out-of-source inside your binary build trees, leaving your source directories pristine.

### Include Guidelines

All generated headers are exported under the unified `aipkg` naming scheme. The resulting structural aliases use native `AiPkg` naming styles:

```cpp
#include <aipkg/ledger_block.hpp>
#include <aipkg/ledger_tx.hpp>

void verifyBlock(const AiPkgBlock& block) {
    uint64_t currentHeight = block.height;
    // Processing continues directly via zero-copy binary arrays...
}

```

### System Interdependencies

* **Code-Gen Backend:** `job_serializer`, `job_serializer_msgpack`, `msgpack-cxx`
* **Cryptographic Layer:** `qt-sodium` / `qsodium` (providing core BLAKE2b hashing and Ed25519 signature validation primitives)
* **Transport/Storage Layer:** `qt-zstd` / `qzstd` (handles the encapsulation of transaction frames inside streaming compressed `Packages.zst` files)
