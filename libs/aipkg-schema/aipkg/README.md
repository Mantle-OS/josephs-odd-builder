# aipkg_pkg — Package Catalog and Manifest Schemas

This directory contains the structural layout schemas defining local manifests, filesystems, and dependency states for the `QtAI` package management system. These definitions are compiled out-of-source via `job_msg_gen` 
into zero-copy, MessagePack-backed C++ data structures.

While the neighboring `ledger/` directory manages timeline immutability and authorization state records, 
the `pkg/` directory defines the physical layout, local installation topologies, and dependency rules of individual AI models, weights, and processing pipelines.

---

## Core Architectural Layout

The layout is built out across four core components:

### 1. File Allocation Layer
* **`pkg_file.yaml`** (`AiPkgFile`): The atomic unit of tracking. Manages specific weight filenames (e.g., `model.safetensors`), tracking their absolute post-installation sizes and local status flags.

### 2. Repository Mapping
* **`pkg_package.yaml`** (`AiPkgPackage`): Links component classifications (UNET, VAE, Text Encoder, LoRA) directly to upstream providers (Hugging Face, GitHub). It handles explicit repository commit tracking via revision hashes, 
ensuring absolute environment consistency.

### 3. Dependency Relations
* **`pkg_depends.yaml`** (`AiPkgDepends`): A foundational tracking link pairing target names to specific version scopes, enabling modular neural architectures to be cleanly shared across multiple workflows.

### 4. Master Project Catalog
* **`pkg_cache.yaml`** (`AiPkgCache`): The top-level orchestration manifest. Compiles metadata, project configurations, UI filtering tags (`low_vram`, `txt2img`), package manifests, and dependency hierarchies into a single comprehensive database index.

---

## System Integration Note

All elements use the uniform `aipkg` include prefix and map directly into native C++ structures using the standard `AiPkg` naming format. 

These components are parsed by the client installation runtime to calculate system storage requirements, verify repository states against remote revisions, 
and automatically organize downloaded assets into their correct runtime target directories.
