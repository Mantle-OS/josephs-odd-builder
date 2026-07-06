# Joseph's Odd Builder

Job or Joseph's odd builder is a c++20 **gnu/linux only** playground for creating userland 
libraries and applications. I use it to test out ideas and algorithms. 

## Job's core library 

The foundational utility layer for the entire Job ecosystem, job_core provides the essential primitives 
required for high performance systems programming. 
It (tries to) establishes a unified execution environment by providing cache-aligned memory allocators, 
high-resolution timers for schedulers, and a standardized logging framework. 
Beyond utilities like CRC32 and PRNG seeding, it defines the IODevice abstraction—the critical interface shared by the IO and UART libraries to ensure consistent data 
handling across different hardware backends.

[core overview](docs/job/core_overview.md)

---

## Job's threading library
job_threads library is a high-performance execution engine designed for diverse, compute-heavy workloads. 
Unlike (some)standard thread pools, it features a policy driven architecture supporting 
FIFO,Round Robin, Work Stealing, and Deadline-aware (Sporadic) scheduling. 
This flexibility allows the system to prioritize fairness or throughput depending on the task. 
The library integrates an epoll-backed async IO event loop directly into the threading context, 
consolidating periodic timers and file descriptor readiness into a single pathway to prevent thread sprawl. 
Beyond task management, it provides a robust suite of parallel utilities, including structured 
parallelism (Parallel For/Reduce), Actor-model pipelines with backpressure, and specialized scientific solvers 
for Monte Carlo simulations and parallel graph traversals (BFS/SSSP).

[threading overview](docs/job/threading_overview.md)

---

## Job's SIMD library 
job_simd library acts as the hardware-abstraction layer that allows the rest of the engine to 
perform data-parallel operations without getting bogged down in platform-specific intrinsics. 
It provides a unified SIMD interface that automatically maps to AVX2, AVX-512, or NEON based on the hardware 
detected at compile-time. By wrapping 256-bit (x86) and 128-bit (ARM) vectors into a consistent API, 
it enables high-throughput float operations, fast math approximations (exp, log, sin), 
and vector transpositions while maintaining a single codebase for the higher-level AI and Science modules.

[simd overview](docs/job/simd_overview.md)

---

## Job's IO library 
job_io library provides a versatile, unified interface for data streaming and persistence, 
extending the IODevice abstraction defined in the Core. 
It is designed to treat diverse data sources—from standard files and system streams to complex 
pseudo-terminals—as consistent, callback-driven objects. 
By integrating with job_threads, the library supports non-blocking, asynchronous operations. 
Which facilitates sub-process execution and shell integration. Additionally, it features a 
high-performance Shared Memory implementation using POSIX primitives and semaphores for fast, 
cross-process producer-consumer communication via a ring-buffer architecture.

[IO overview](docs/job/io_overview.md)

---

## Job's UART library 
job_uart library provides a framework for serial port communication and hardware discovery. 
By extending the IODevice interface and leveraging libudev, it allows for the seamless identification and 
management of physical devices. The library features a robust SerialManager that automatically 
tracks system-level udev events—such as plugging or unplugging a device—to maintain an accurate map of 
available ports. For data transfer, it utilizes an epoll-backed async thread from the threading library, 
ensuring that high-speed serial streams are handled via non-blocking callbacks without stalling a 
main application.

[Uart overview](docs/job/uart_overview.md)

---

## Job's networking library 
job_net library provides a networking stack designed for low-latency communication across various protocols. 
It builds upon the job_threads epoll-backed event loop to drive asynchronous socket readiness, 
ensuring that network operations remain non-blocking. The library offers a unified interface for TCP, UDP, and Unix Domain Sockets, 
complemented by a suite of utility classes for URL parsing, IP address management (IPv4/IPv6), 
and HTTP header manipulation.

[networking overview](docs/job/net_overview.md)

---

## Job's serializer library 
job_serializer library is a data-modeling and code-generation framework designed to bridge the gap between 
abstract schemas and high-performance C++ implementations. 
It allows developers to define message structures in YAML or JSON and automatically generate type-safe 
C++ structs complete with serialization logic. The library supports a Dynamic Runtime Object 
for generic message handling and a pluggable backend system that includes built-in support 
for JSON, YAML, and a binary MsgPack (plugin) implementation.

[serializer overview](docs/job/serializer_overview.md)

---

## Job's AI library
job_ai library is a vertically integrated, high-performance AI engine that diverges from standard deep 
learning frameworks. By replacing backpropagation with Forward-Only Evolutionary Synthesis 
and utilizing physics-informed execution strategies, job_ai achieves hardware-native speeds without 
the memory overhead of traditional training stacks. It treats neural weights as particle systems, 
leveraging specialized adapters like Barnes-Hut (BH) and Fast Multipole Method (FMM) to scale attention 
and dense layers across high-end SIMD hardware (AVX-512, NEON).

[deeper dive into Job AI ](docs/job/ai.md)

---

## Job's science library

job_science library provides the computational physics, focusing on simulation 
data structures and numerical solvers. 
It bridges the gap between raw data and physical realization, 
offering a suite of integrators (Verlet, RK4, Euler) and advanced N-body interaction 
algorithms like Barnes–Hut and the Fast Multipole Method (FMM). 
Designed for massive scale, the library integrates directly with job_threads to parallelize 
force calculations and stencil grid updates across high-core-count systems, 
making it ideal for planetary disk modeling and complex particle simulations.

[science document](docs/job/science_overview.md)

---

## Job's Ansi library
job_ansi library is a terminal emulation engine designed to parse ansi byte streams into a structured, 
renderable screen model. 
Unlike simple string-to-ANSI converters, job_ansi implements a state-aware parser capable of handling 
incremental UTF-8 decoding, CSI/OSC/ESC escape sequences, 
and specialized charset designators. It manages a complete virtual terminal state, 
including primary and alternate buffers, a bounded scrollback deque, and a dirty rectangles tracking system 
for optimized incremental rendering. It supports mouse reporting, bracketed paste, and Xterm extensions, 
it provides the logic required to build modern terminal emulators Or TUI frameworks.....

[ansi highlevel documentation](docs/job/ansi_overview.md)

---


## Job's TUI library

job_tui library is a "declarative" style Terminal User Interface framework that brings modern GUI paradigms 
like item trees, anchors, and layout engines directly to the terminal.  Very much like Qml

Building upon the job_ansi and job_threads stack, 
it provides a full-featured application lifecycle that handles raw mode transitions, 
event routing (keyboard and SGR mouse), and efficient incremental rendering. With a 
rich library of widgets (Buttons, Labels, Checkboxes) and layout containers (Windows, Grids, Panels).

job_tui allows for the creation of complex, interactive terminal applications with (kinda) minimal boilerplate.


[tui high level doc](docs/job/tui_overview.md)


---

## Job's crypto library

job_crypto is the cryptographic foundation of the job stack, 
built on libsodium with no other job::* library dependencies. 

It covers secure memory (locked, zeroed-on-release buffers), 

asymmetric keypair management and Ed25519 file signing, BLAKE2b hashing, 

symmetric authenticated encryption, and Argon2id password hashing and key derivation. 

Every other library that needs real cryptography job_zstd's encrypted archives, job_net's password-protected URLs 

builds on top of job_crypto rather than duplicating any of this.

[This doc goes into more detail.](docs/job/crypto_overview.md)

---


## Job's Zstd library

job_zstd is a pure C++ compression library built on Zstandard, with no Qt dependency at all. It compresses single files and whole directory trees into a single self-describing archive, where every entry (a file, a directory, an empty directory, or a symlink) carries its own type tag, so an archive never needs outside context to know what it contains.

The library builds directly on job_crypto for its cryptographic layer. Encryption and detached Ed25519 signing are not bolted on as an afterthought, they are treated as first-class operations, since job_zstd is meant to serve as the transport format for the aipkg package ledger system, where the contents of an archive need to be verifiable, not just recoverable.

Compression and decompression are implemented as std::streambuf based transports layered on top of one another, which is what allows encryption to sit as an outer wrapper around an otherwise ordinary compressed stream without either side needing to know about the other. The library also includes hardening against path traversal and symlink based extraction attacks, since it may end up extracting archives whose contents were not necessarily authored by someone already trusted.

[This doc goes into more detail.](docs/job/zstd_overview.md)

---


