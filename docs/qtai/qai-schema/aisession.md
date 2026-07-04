# Session Schemas (`aisession`)

The `aisession` schemas define the authenticated user session model for the QtAI ecosystem.

Unlike the package (`aipkg`) and ledger (`ailedger`) schemas, which describe distributable content and cryptographic trust, the session schemas describe **local user identity**, **credential management**, and **encrypted runtime state**.

These schemas are consumed by `job_msg_gen` to generate strongly-typed C++ structures backed by MessagePack serialization. The resulting data is compressed using **QZstd** and encrypted using **QSodium** before being written to disk.

---

## Purpose

The session system provides a common storage format for managing:

- Local users
- Authentication metadata
- Cryptographic key pairs
- API credentials
- Provider configuration
- Encrypted user vaults

This allows QtAI applications to share a single authenticated session while keeping all sensitive material inside an encrypted vault.

---

## Core Architectural Layout

The session schemas are organized into three logical layers.

### 1. User Management

These schemas describe local users and provide the information required to locate and unlock encrypted vaults.

* **`session_user.yaml`** (`AiSessionUser`) — Local user account metadata including password verification information and vault location.
* **`session_user_index.yaml`** (`AiSessionUserIndex`) — Top-level index of all local users known to the system.

---

### 2. Identity & Credentials

These schemas describe the authenticated identity of a user after a vault has been unlocked.

* **`session_key.yaml`** (`AiSessionKey`) — Cryptographic identity, package signing, and ledger signing keys.
* **`session_credential.yaml`** (`AiSessionCredential`) — Authentication secrets such as API keys, bearer tokens, and other provider credentials.
* **`session_provider.yaml`** (`AiSessionProvider`) — Configured provider accounts that reference stored credentials.

---

### 3. Secure Vault

The vault aggregates all authenticated state into a single encrypted object.

* **`session_vault.yaml`** (`AiSessionVault`) — Master encrypted vault containing keys, credentials, provider configuration, and user metadata.

---

## Runtime Organization

The session subsystem follows a layered design.

```text
AiSessionUserIndex
        │
        ▼
AiSessionUser
        │
        ▼
Password Verification (Argon2id)
        │
        ▼
Vault Key Derivation
        │
        ▼
AiSessionVault
        │
        ├── Keys
        ├── Credentials
        └── Providers
```

The user index remains readable so applications can present available accounts, while the vault itself remains encrypted until authentication succeeds.

---

## Storage Pipeline

Session objects are generated from the YAML schemas into strongly typed MessagePack structures.

Before being persisted, the vault passes through the QtAI secure storage pipeline:

```text
AiSessionVault
        │
        ▼
MessagePack Serialization
        │
        ▼
QZstd Compression
        │
        ▼
QSodium Encryption
        │
        ▼
Encrypted Vault File
```

Loading a session performs the reverse operation:

```text
Encrypted Vault
        │
        ▼
QSodium Decryption
        │
        ▼
QZstd Decompression
        │
        ▼
MessagePack Deserialization
        │
        ▼
AiSessionVault
```

---

## Secure Memory

Sensitive material stored inside the vault is intended to remain encrypted on disk at all times.

After successful authentication, secrets are loaded into protected runtime memory before use.

This includes:

- private signing keys
- API tokens
- bearer tokens
- authentication credentials

The session runtime is designed to integrate with secure memory containers (such as `QSecureMem`) so sensitive data spends as little time as possible in ordinary process memory.

