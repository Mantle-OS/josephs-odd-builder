# Joseph's Odd Builder

Joseph's Odd Builder, or **JOB**, is a C++20(thanks nvidia) GNU/Linux playground for building userland libraries, runtimes, tools, simulations, and applications.

The project is split into two major documentation areas:

## JOB Core Ecosystem

Low-level runtime libraries for threading, IO, networking, SIMD, crypto, serialization, ANSI/TUI, science simulation, and experimental AI.

* [JOB documentation](docs/job/job.md)

## QtAI Ecosystem

Qt/QML-facing AI infrastructure built on top of JOB and native runtimes such as `llama.cpp`, `stable-diffusion.cpp`, libsodium, zstd, and Hugging Face APIs.

* [QtAI documentation](docs/qtai/qtai.md)

## Building

See the JOB documentation for build dependencies, options, and module-specific notes.

## Status

This is an experimental systems playground. APIs are allowed to change while the architecture is still being shaped.
