# `job_schema_example` Design

> **Document purpose:** This is a design and implementation-planning document for `apps/job_schema_example`.
>
> It records the purpose, structure, build flow, schema inputs, generated-library expectations, and verification goals of the JOB Schema example application.
>
> It is not API reference documentation and should not be treated as a promise of exact generated class APIs, filenames, command-line options, or final shorthand macro spellings.
>
> For the overall JOB Schema architecture, see [JOB Schema — High-Level Architecture](intro.md).
>
> For schema semantics and generation policy, see [`job_schema` Design](job_schema_design.md).
>
> For generator behavior, see [`job_schema_gen` Design](job_schema_gen_design.md).
>
> For test strategy, see [`test_job_schema` Design](test_job_schema_design.md).
>
> For dependency and CMake integration, see [JOB Schema Dependencies and Build Integration](job_schema_design_deps.md).

---

# Purpose

`job_schema_example` is the end-to-end usage example for JOB Schema.

Its job is simple:

> **Show how a normal JOB project declares schemas, generates a library from them, links that generated library, and uses the generated C++ types.**

The example should exercise the public developer workflow rather than internal generator APIs.

Conceptually:

```text
C++ schema files
      ↓
job_schema_gen_library(...)
      ↓
job_schema_gen
      ↓
target compiler + C++26 reflection
      ↓
generated C++ library
      ↓
job_schema_example
      ↓
normal application use
```

The application should remain intentionally small.

It is not a generator test harness disguised as an example.

It is a consumer.

---

# Relationship to the Legacy Generator Example

The example should deliberately resemble the previous `job_msg_gen` demonstration.

Historically, the example build looked conceptually like:

```cmake
cmake_minimum_required(VERSION 3.16)

set(TARGET_NAME "job_msg_gen")

project(${TARGET_NAME}
    VERSION 0.1
    DESCRIPTION "use the message pack compiler too make a lib"
    LANGUAGES CXX
)

job_msgpack_gen_lib(
    NAME ${TARGET_NAME}
    VERSION 0.1
    DESCRIPTION "use the message pack compiler too make a lib"

    EXPORT_HEADER jobmsggen_export.h
    EXPORT_MACRO JOBMSGGEN_EXPORT

    SCHEMA_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/input/example_req.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/example_user.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/example_tx.json
)
```

The new example should intentionally keep a similar build shape:

```cmake
cmake_minimum_required(VERSION 3.16)

set(TARGET_NAME "job_schema_example")

project(${TARGET_NAME}
    VERSION 0.1
    DESCRIPTION "JOB Schema generation example"
    LANGUAGES CXX
)

job_schema_gen_library(
    NAME ${TARGET_NAME}_generated
    VERSION 0.1
    DESCRIPTION "Generated JOB Schema example library"

    EXPORT_HEADER jobschemaexample_export.h
    EXPORT_MACRO JOBSCHEMAEXAMPLE_EXPORT

    SCHEMA_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/input/example_req.h
        ${CMAKE_CURRENT_SOURCE_DIR}/input/example_user.h
        ${CMAKE_CURRENT_SOURCE_DIR}/input/example_tx.h
)
```

The exact CMake function arguments remain subject to the build-integration design.

The important point is continuity:

```text
old
    external schema files
    ↓
    generator function
    ↓
    generated C++ library

new
    real C++ schema files
    ↓
    generator function
    ↓
    generated C++ library
```

The user-facing workflow remains familiar while the schema language itself becomes dramatically simpler.

---

# Example Goals

The example should demonstrate:

```text
declaring a schema in C++
using job_schema shorthand
using more than one schema input file
referencing one schema-generated type from another
generating a library through CMake
linking the generated library
constructing generated types
reading/writing generated properties where applicable
using nested generated objects
building against normal JOB libraries
```

It should also make the reduction in repeated metadata obvious.

---

# Example Schema Set

The initial example should preserve the same conceptual data models used by the old generator:

```text
ExampleReq
ExampleUser
ExampleTx
```

This is intentional.

Keeping the conceptual models the same makes the architectural difference easy to compare.

---

# `ExampleReq`

The old external schema conceptually described:

```text
ExampleReq
    request_id : uint64
    payload    : variable binary data
```

The new JOB Schema input should express that directly in C++.

Conceptually:

```cpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <job_schema_alias.h>

JOB_BaseObj(ExampleReq,
    JOB_RW(std::uint64_t,          requestId, 0)
    JOB_RW(std::vector<std::byte>, payload,   {})
)
```

The exact shorthand spelling remains subject to the public alias design.

The important point is what is no longer present.

There is no need to write:

```text
tag
version
unit
base string
C struct symbol
include prefix
output base filename
field type strings
```

because those facts either:

```text
already exist in the C++ declaration
```

or:

```text
belong to the build/generator rather than the schema
```

---

# `ExampleUser`

The old generator needed explicit metadata to describe a nested structure.

Conceptually:

```yaml
- name: "request"
  type: "struct"
  ref_include: "example_req.hpp"
  ref_sym: "ExampleReq_t"
```

JOB Schema should use normal C++ type relationships instead.

Conceptually:

```cpp
#pragma once

#include <cstdint>
#include <string>

#include <job_schema_alias.h>

#include "example_req.h"

JOB_BaseObj(ExampleUser,
    JOB_RW(std::int32_t, userId,   0)
    JOB_RW(std::string,  username, {})
    JOB_RW(ExampleReq,   request,  {})
)
```

The compiler already knows:

```text
the type is ExampleReq
where ExampleReq is declared
its namespace
its structure
its generated/schema identity
```

The schema should not repeat those facts using strings.

---

# Nested Types

The nested `ExampleReq` member exists specifically to prove that JOB Schema follows normal C++ relationships.

The expected authoring model is:

```cpp
#include "example_req.h"

JOB_RW(ExampleReq, request, {})
```

not:

```text
type = "struct"
include = "example_req.hpp"
symbol = "ExampleReq_t"
```

This is one of the most important example cases because it demonstrates the central JOB Schema rule:

> **If C++ already knows the relationship, the schema should not describe it again.**

---

# `ExampleTx`

The old JSON example conceptually represented:

```text
ExampleTx
    from_addr : 32-byte array
    to_addr   : 32-byte array
    amount    : uint64
```

JOB Schema should describe that using actual C++ types.

Conceptually:

```cpp
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <job_schema_alias.h>

JOB_BaseObj(ExampleTx,
    JOB_RW(std::array<std::byte, 32>, fromAddr, {})
    JOB_RW(std::array<std::byte, 32>, toAddr,   {})
    JOB_RW(std::uint64_t,             amount,   0)
)
```

There is no generator-specific type spelling such as:

```text
bin[32]
```

The type is:

```cpp
std::array<std::byte, 32>
```

because that is what the application actually means.

---

# Documentation Is Not Schema Data

The old schema formats allowed metadata such as:

```yaml
comment: "A variable-length binary payload."
```

JOB Schema intentionally does not translate this into another schema feature.

There should not be generated-schema documentation mechanisms such as:

```cpp
[[=job::schema::class_doc{"..."}]]
```

or:

```cpp
[[=job::schema::member_doc{"..."}]]
```

or additional shorthand arguments such as:

```cpp
JOB_BaseObj(Type, "documentation", ...)
```

or:

```cpp
JOB_RW(Type, member, defaultValue, "documentation")
```

Documentation strings are not schema-generation policy.

If documentation is needed, it belongs in the library or application that gives the generated object meaning.

Normal source comments remain valid C++:

```cpp
/// Variable-length binary payload.
JOB_RW(std::vector<std::byte>, payload, {})
```

Developers may use Doxygen or another documentation system against source comments if desired.

`job_schema` does not preserve, interpret, serialize, or generate schema metadata from those comments.

The rule is:

> **Documentation may exist beside the schema, but documentation is not part of the schema.**

---

# Schema Files Are Small

The example should make it immediately obvious that using C++ does not make the schema more verbose than the old external formats.

In practice it should be smaller.

The old external declaration often had to repeat:

```text
logical type name
generated C symbol
base type
unit
include prefix
output filename
member names
member type strings
reference include
reference symbol
required flags
comments
```

The new schema should mostly contain:

```text
generated kind
actual C++ types
actual member names
generation policy
defaults
```

The difference is not merely fewer lines.

The more important difference is:

> **There is less information to keep synchronized.**

---

# No Custom Type Vocabulary

The example should demonstrate that JOB Schema does not invent aliases such as:

```text
u64
i32
str
bin
bin[32]
struct
```

to represent C++ types.

The schema uses:

```cpp
std::uint64_t
std::int32_t
std::string
std::vector<std::byte>
std::array<std::byte, 32>
ExampleReq
```

The compiler already understands those types.

JOB Schema should not create another vocabulary that maps back into them.

---

# No Generated Symbol Metadata

The example schema should not contain metadata equivalent to:

```text
c_struct
ref_sym
tag
out_base
```

The C++ declaration already has a name.

The compiler already knows its namespace.

The build system already knows the schema source filename and generated target.

Output naming and generated-file policy belong to generation/build integration rather than individual schema members.

---

# No Include Metadata

A nested type should use normal:

```cpp
#include "example_req.h"
```

when that declaration is required.

There should not be schema metadata equivalent to:

```text
ref_include
include_prefix
```

merely to describe an ordinary C++ dependency.

---

# Generated Library

The CMake function should produce one normal generated library target.

Conceptually:

```cmake
job_schema_gen_library(
    NAME job_schema_example_generated

    SCHEMA_FILES
        input/example_req.h
        input/example_user.h
        input/example_tx.h
)
```

The resulting target should behave like an ordinary CMake library target for consumers.

The application should be able to link it normally:

```cmake
target_link_libraries(job_schema_example PRIVATE
    job_schema_example_generated
)
```

The exact generated target naming convention remains part of the CMake integration design.

---

# Example Application

The example application's own source should be intentionally ordinary.

It should not call internal schema reflection helpers or generator APIs.

Conceptually:

```cpp
#include <example_req.h>
#include <example_tx.h>
#include <example_user.h>

int main()
{
    // Construct generated types.
    // Exercise generated API.
    // Exercise nested ExampleReq inside ExampleUser.
    // Verify normal application use.

    return 0;
}
```

The exact member accessor names should be added only after the generated API is implemented.

The example design should not invent final method signatures ahead of implementation.

---

# The Example Is a Consumer

This distinction is important.

`job_schema_example` is not responsible for proving every invalid schema case.

That belongs in:

```text
tests/job_schema
```

The example should not contain a giant set of:

```text
static_assert failures
invalid declarations
compiler-failure fixtures
generator-internal checks
```

Its purpose is to answer:

> **What does a normal developer actually write?**

and:

> **What does a normal application look like after generation?**

---

# Relationship to `tests/job_schema`

The responsibilities are intentionally different.

```text
tests/job_schema
    exhaustive correctness
    compile-failure tests
    edge cases
    policy combinations
    Packed layout cases
    reflection behavior
    benchmarks/stress

apps/job_schema_example
    normal usage
    generated-library integration
    realistic schema declarations
    normal consumer code
```

Both may use similar model types, but their jobs are different.

---

# Build Flow

The example build should demonstrate the complete public generation path:

```text
input/example_req.h
input/example_user.h
input/example_tx.h
        │
        ▼
job_schema_gen_library(...)
        │
        ▼
job_schema_gen
        │
        ▼
target compiler
        │
        ▼
C++26 reflection + generation
        │
        ▼
generated C++
        │
        ▼
generated library
        │
        ▼
job_schema_example executable
```

The example should never need to manually invoke the internal helper or compiler stages.

Those are implementation details hidden behind the CMake function.

---

# CMake Experience

The CMake integration should intentionally remain similar in spirit to the legacy generator.

The developer says:

```text
here is the generated library
here are its schema source files
```

and the build machinery handles the rest.

A conceptual complete example may look like:

```cmake
cmake_minimum_required(VERSION 3.16)

set(TARGET_NAME "job_schema_example")

project(${TARGET_NAME}
    VERSION 0.1
    DESCRIPTION "JOB Schema generation example"
    LANGUAGES CXX
)

job_schema_gen_library(
    NAME ${TARGET_NAME}_generated
    VERSION 0.1
    DESCRIPTION "Generated JOB Schema example library"

    EXPORT_HEADER jobschemaexample_export.h
    EXPORT_MACRO JOBSCHEMAEXAMPLE_EXPORT

    SCHEMA_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/input/example_req.h
        ${CMAKE_CURRENT_SOURCE_DIR}/input/example_user.h
        ${CMAKE_CURRENT_SOURCE_DIR}/input/example_tx.h
)

add_executable(${TARGET_NAME}
    main.cpp
)

target_link_libraries(${TARGET_NAME} PRIVATE
    ${TARGET_NAME}_generated
)
```

The final function spelling and optional arguments may change.

The desired developer experience should not.

---

# Schema Input Directory

The example should keep its schema inputs together.

A likely layout is:

```text
apps/job_schema_example/
├── CMakeLists.txt
├── main.cpp
└── input/
    ├── example_req.h
    ├── example_user.h
    └── example_tx.h
```

Generated files belong in the build tree.

They should not be checked into the source tree merely to make the example work.

---

# Generated Files Are Build Outputs

The example source tree contains:

```text
schema inputs
consumer application source
CMake configuration
```

It should not contain manually maintained copies of generated:

```text
headers
sources
export files
temporary helpers
```

Those are build products.

This makes the example useful for proving generation from a clean checkout.

---

# Clean-Build Requirement

The example should build from a clean build directory with no pre-generated source available.

That proves the actual dependency path works.

Conceptually:

```text
git checkout
    ↓
empty build directory
    ↓
cmake configure
    ↓
ninja
    ↓
schema generation occurs
    ↓
generated library builds
    ↓
example application builds
```

If the example only works because generated files are already present in the repository, it has failed its purpose.

---

# Generated Include Experience

The example application should include generated output as ordinary public headers.

It should not need to know:

```text
temporary helper paths
generator staging directories
compiler scratch directories
```

CMake should expose the correct generated include directory through the generated target.

The application should consume it exactly as it would consume a normal JOB library.

---

# Native Generation

On a normal native build:

```text
host compiler == executable target
```

the example should demonstrate:

```text
compile temporary generation helper
run helper directly
compile generated library
compile consumer application
```

No special user action should be required.

---

# Cross Generation

The example design should remain compatible with cross compilation.

When target-generated helper execution requires an emulator:

```text
cross compiler
    ↓
target helper
    ↓
configured emulator
    ↓
generated source
```

the example application should still be built through the same public CMake function.

The example itself should not contain QEMU-specific logic.

That belongs to the generation/build infrastructure.

---

# Comparison With the Legacy Example

The example should retain a short comparison because it demonstrates one of the primary reasons JOB Schema exists.

## Legacy `ExampleReq`

Conceptually:

```yaml
tag: "ExampleReq"
version: 1
unit: "example"
base: "BaseStruct"
c_struct: "ExampleReq_t"
include_prefix: "job_example"
out_base: "example_req"
fields:
  - name: "request_id"
    type: "u64"
    required: true
  - name: "payload"
    type: "bin"
```

## JOB Schema `ExampleReq`

Conceptually:

```cpp
#include <job_schema_alias.h>

JOB_BaseObj(ExampleReq,
    JOB_RW(std::uint64_t,          requestId, 0)
    JOB_RW(std::vector<std::byte>, payload,   {})
)
```

The new declaration is not merely shorter.

It removes duplicated facts.

---

# What Was Removed

The example should explicitly explain where the old metadata went.

| Legacy metadata   | JOB Schema                                         |
| ----------------- | -------------------------------------------------- |
| `tag`             | C++ type name already provides identity            |
| `version`         | Not inherently part of a C++ schema declaration    |
| `unit`            | Build/library concern rather than member structure |
| `base`            | Selected directly by schema kind                   |
| `c_struct`        | C++ declaration already has a type name            |
| `include_prefix`  | Normal C++/build include handling                  |
| `out_base`        | Generator/build output policy                      |
| `type: "u64"`     | `std::uint64_t`                                    |
| `type: "str"`     | `std::string`                                      |
| `type: "bin"`     | normal C++ byte container                          |
| `type: "bin[32]"` | `std::array<std::byte, 32>`                        |
| `type: "struct"`  | actual C++ nested type                             |
| `ref_include`     | normal `#include`                                  |
| `ref_sym`         | actual C++ type name                               |
| `comment`         | not schema data                                    |

The goal is not to translate every old key into a new feature.

Many of those keys disappear because they are no longer necessary.

---

# Why the New Example Is Better

The old generator had to maintain a mapping from an external schema language into C++.

That required custom concepts such as:

```text
u64
i32
str
bin
bin[32]
struct
ref_include
ref_sym
```

JOB Schema starts with C++.

Therefore:

```text
there is no custom type language
there is no second namespace system
there is no second include system
there is no second nested-type reference system
there is no separate field-name string
there is no generated-symbol string
```

The schema contains only information required to express:

```text
what C++ type is this?
what members does it contain?
what generated policy should apply?
what is the default?
```

Everything else is derived.

---

# A Familiar Shape, Not a Familiar Implementation

The example intentionally resembles the old `job_msg_gen` example at the project level:

```text
schema files
    ↓
CMake generation function
    ↓
generated library
    ↓
application
```

That familiarity is useful.

The internals are completely different.

Old:

```text
external data format
    ↓
custom parser
    ↓
custom type mapping
    ↓
C++ generator
```

New:

```text
C++ declaration
    ↓
C++ compiler
    ↓
C++26 reflection
    ↓
generated C++
```

The user workflow stays recognizable while the duplicated language disappears.

---

# Example Verification

The example should verify enough behavior to prove it is real.

At minimum, the executable should successfully:

```text
construct generated types
set RW members
read generated members
use nested ExampleReq from ExampleUser
use fixed-size ExampleTx address members
exercise default values
link and run against the generated library
```

If BaseObject persistence is already available when the example is implemented, a small serialization/reconstruction demonstration may also be useful.

The example should not grow into exhaustive persistence testing.

That belongs in `tests/job_schema` and `job_core`.

---

# Build Verification

The example is successful when:

```text
CMake configures
schema generation executes
generated source compiles
generated library links
example application compiles
example application links
example application runs successfully
```

This provides a simple end-to-end integration target suitable for development and CI.

---

# Product Requirements

The implementation of `apps/job_schema_example` should satisfy the following requirements.

1. It must demonstrate the public JOB Schema workflow rather than internal generator APIs.

2. It should preserve the conceptual `ExampleReq`, `ExampleUser`, and `ExampleTx` examples from the legacy generator.

3. Schema inputs must be real C++.

4. The example must use the public JOB Schema shorthand layer intended for normal developers.

5. The example must not use YAML or JSON schema inputs.

6. The example must demonstrate multiple schema source files.

7. The example must demonstrate a nested generated type relationship.

8. Nested types must use normal C++ includes and type names.

9. The example must not use custom type strings such as `u64`, `bin`, or `struct`.

10. The example must not introduce replacements for legacy metadata that C++ or the build already provides.

11. Documentation strings must not become schema annotations or macro arguments.

12. Normal C++/Doxygen comments may exist in schema source, but JOB Schema does not treat them as schema data.

13. The generated library must be created through the public CMake generation function.

14. The example executable must link the generated library as an ordinary CMake target.

15. Generated headers and sources must remain build outputs rather than checked-in example source.

16. The example must build from a clean source/build state.

17. The application must consume generated headers through the generated target's public include interface.

18. The application must not depend on temporary helper/compiler paths.

19. Native generation must require no manual helper invocation.

20. Cross generation must remain possible through the same public build interface.

21. The example should remain intentionally small and readable.

22. Exhaustive invalid-schema and edge-case testing belongs in `tests/job_schema`, not the example application.

23. The example should visibly demonstrate the reduction in duplicated schema information compared with the legacy generator.

24. The example should remain useful as practical documentation for application developers.

---

# Decisions Already Made

The following are current design decisions:

```text
job_schema_example is a consumer example.

It mirrors the overall shape of the old job_msg_gen example.

Its schema inputs are C++ rather than YAML/JSON.

ExampleReq, ExampleUser, and ExampleTx remain the conceptual examples.

The public shorthand layer is used in example schema files.

Normal C++ types replace generator-specific type strings.

Normal #include relationships replace ref_include metadata.

Normal C++ type names replace ref_sym metadata.

Documentation comments are not schema data.

Legacy comment metadata is removed rather than translated.

Generated code lives in the build tree.

The generated library is consumed as an ordinary CMake target.

The example application does not use generator internals.

Tests and the example have different jobs.

The example should remain small.
```

---

# Open Design Questions

Only example-specific questions should remain here.

## Exact Shorthand Spelling

The example should use the final public shorthand once implemented.

Conceptual forms currently include:

```text
JOB_BaseObj(...)
JOB_RW(...)
JOB_RO(...)
JOB_CONST(...)
```

The design does not freeze those exact spellings before the alias layer is implemented.

---

## Required Shorthand

Some legacy fields were marked required.

The final example should use the public shorthand or composition mechanism chosen for:

```text
RW + Required
```

without inventing a temporary syntax solely for the example.

---

## Generated Target Name

Should the generated target be named:

```text
job_schema_example_generated
```

or use another naming convention derived automatically from the CMake function?

The build-integration design should answer this consistently for all generated libraries.

---

## Runtime Demonstration

How much behavior should `main.cpp` print or assert?

A small amount is useful.

The example should not become a test runner.

---

## Persistence Demonstration

Should the first example include a very small BaseObject persistence round trip?

This is useful only if the persistence integration is already stable when the example is implemented.

It should not block the initial example.

---

# Design Principles

The example should continually be judged against a few rules:

> **Show normal developer code, not generator internals.**

> **Do not repeat information the compiler already knows.**

> **Do not replace removed YAML/JSON metadata with new C++ metadata merely to preserve old concepts.**

> **Use normal C++ for types, includes, defaults, and relationships.**

> **Keep the example small enough that a developer can understand the complete workflow quickly.**

> **Let tests prove edge cases; let the example prove usability.**

The most important comparison is:

```text
old
    describe C++ to a generator

new
    write C++
```

And the desired developer experience is:

```text
write a small schema
      ↓
list it in CMake
      ↓
build
      ↓
use a normal generated JOB library
```

Nothing more should be required.

That is the example's one job.
