# QHF

QHF provides Qt and QML classes for working with the Hugging Face Hub.

It provides access to the Hugging Face REST API, local package metadata, user management, and repository information. The library is designed to integrate with the other QtAI libraries, including `qt-ai-utils`, `qt-sodium`, and `qt-zstd`.

The current implementation focuses on repository discovery and local metadata management. Package installation, session management, and repository synchronization are still under development.

## Components

### HuggingFaceHub

The main entry point into the library.

`HuggingFaceHub` owns the major runtime objects used by the library, including:

* `HuggingFaceApi`
* `HuggingFaceCache`
* `HuggingFaceUserManager`
* `QDownloader`

Applications typically interact with the library through this class.

### HuggingFaceApi

Provides access to the Hugging Face REST API.

Current API support includes operations such as:

* User authentication (`whoami`)
* Trending repositories
* Model search
* Repository information
* Repository references
* Repository trees
* Repository commits
* LFS file information

All requests are asynchronous and return `QFuture<QJsonObject>`.

### HuggingFaceCache

Maintains a local cache of repository metadata.

The cache stores `HuggingFacePackageManifest` objects and serializes them to a compressed archive on disk using Zstandard.

### HuggingFacePackageManifest

Represents a single Hugging Face repository.

It stores metadata such as:

* Repository id
* Author
* Pipeline tag
* License
* SHA
* Likes
* Repository state
* File list
* Branch list

Repository files and branches are exposed through `ObjectListModel` instances for use from Qt and QML.

### HuggingFaceFileManifest

Represents a file contained within a repository.

The class tracks the repository filename together with local installation state and output location.

### HuggingFaceRepoBranch

Represents a repository branch or reference returned by the Hugging Face API.

### HuggingFaceUser

Represents a Hugging Face user profile together with locally stored authentication metadata.

The class stores account information returned by the Hugging Face API together with encrypted local credential data used by QtAI.

### HuggingFaceUserManager

Owns the available user profiles and tracks the current active user.

An anonymous user is always available. Additional users can be stored locally and selected as the active user.

Authentication is performed using encrypted local credentials rather than storing raw access tokens.

## Design

QHF separates networking, local storage, and schema objects.

* `HuggingFaceApi` communicates with the Hugging Face REST API.
* `HuggingFaceCache` stores repository metadata locally.
* `HuggingFaceUserManager` manages user profiles and authentication state.
* Manifest classes represent repository, file, and branch metadata.

This separation allows the networking layer, cache, and user management to evolve independently.

## Current Status

The library is currently in an early alpha state.

Repository discovery, metadata parsing, and local caching are implemented. Package installation, session management, and additional package management features are under active development.

## See also

* [qt-ai-utils](docs/qtai/qt-ai-utils.md)
* [qsodium](docs/qtai/qsodium.md)
* [qzstd](docs/qtai/qzstd.md)
