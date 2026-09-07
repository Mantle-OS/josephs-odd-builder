# `job_yaml` Design

> **Document purpose:** This is a design and implementation-planning document for `libs/job_yaml`.
>
> It records the requirements, architectural boundaries, parsing and emission model, reflection integration, supported YAML behavior, performance goals, and open design questions for the library.
>
> It is not API reference documentation and should not be treated as a promise of exact class names, header names, parser types, node types, or implementation details.
>
> For the motivation and high-level architecture, see [JobYaml Design Introduction](intro.md).

---

# Purpose

`job_yaml` is the standalone YAML library for JOB.

Its purpose is to provide YAML parsing and emission without relying on `yaml-cpp`, while taking advantage of the C++26 reflection and type facilities already available throughout JOB.

The primary reflected path should allow YAML to be lowered directly into the final C++ object representation rather than requiring an intermediate YAML object tree.

Conceptually:

```text
YAML
    ↓
grammar / structural parser
    ↓
reflected destination type
    ↓
type-directed conversion
    ↓
C++ object
```

and:

```text
C++ object
    ↓
reflection
    ↓
type-directed YAML emission
    ↓
YAML
```

A dynamic YAML representation may also be provided for applications that genuinely need to inspect, build, or modify arbitrary YAML.

That representation must not become a mandatory intermediate representation for reflected serialization.

The primary design goal is:

> **Use the destination C++ type as much as possible instead of rebuilding information the compiler already knows.**
