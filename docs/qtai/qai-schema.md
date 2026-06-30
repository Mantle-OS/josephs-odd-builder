# QtAI Schema Collection

The QtAI schema collection defines the binary data model used throughout the QtAI ecosystem.

Each schema is authored in YAML and compiled into strongly-typed C++ structures using `job_msg_gen`. The generated code provides zero-copy MessagePack serialization and integrates directly with the QtAI runtime libraries.

Unlike traditional hand-written serialization code, every schema has a single source of truth. The generated structures are shared by applications, services, package management, and cryptographic tooling, ensuring a consistent binary interface across the entire project.

For details on the schema compiler and serialization infrastructure, see:

- [Serializer Overview](docs/job/serializer_overview.md)

---

## Schema Groups

The QtAI ecosystem is organized into three independent schema families.

| Schema | Purpose |
|---------|---------|
| **aisession** | Local user accounts, encrypted vaults, credentials, cryptographic keys, and provider configuration. |
| **ailedger** | Cryptographic transparency ledger, trust records, signatures, attestations, and append-only package history. |
| **aipkg** | AI package manifests, repository metadata, dependency graphs, and installation information. |

Although these schema families work together, they intentionally model different concerns.

They answer three different questions:

| Question | Schema |
|----------|--------|
| **Who am I and how do I authenticate?** | `aisession` |
| **Can I trust this package?** | `ailedger` |
| **What is this package?** | `aipkg` |

This separation allows authentication, package metadata, and trust verification to evolve independently while remaining interoperable through the generated serialization layer.

---

## Session Schemas (`aisession`)

Defines the authenticated user model used by QtAI, including encrypted vaults, credentials, cryptographic keys, and provider configuration.

* [See also aisession overview](docs/qtai/qai-schema/aisession.md)

---

## Ledger Schemas (`ailedger`)

Defines the append-only cryptographic ledger used to establish package authenticity, developer identity, signatures, and transparency proofs.

* [See also ailedger overview](docs/qtai/qai-schema/ailedger.md)

---

## Package Schemas (`aipkg`)

Defines package manifests, repository metadata, dependency relationships, installation layouts, and AI model catalog information.

* [See also aipkg overview](docs/qtai/qai-schema/aipkg.md)

---