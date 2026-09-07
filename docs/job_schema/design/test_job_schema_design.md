# `job_schema_gen.cmake` Design

> **Document purpose:** This is a design and implementation-planning document for `cmake/job_schema_gen.cmake`.
>
> It records the responsibilities, public build shape, toolchain propagation, generated-target behavior, dependency tracking, and architectural boundaries of the JOB Schema CMake integration.
>
> It is not API reference documentation and should not be treated as a promise of exact function arguments, internal custom-command structure, temporary filenames, or final generated target naming.
>
> For the overall JOB Schema architecture, see [JOB Schema — High-Level Architecture](intro.md).
>
> For schema semantics, see [`job_schema` Design](job_schema_design.md).
>
> For generator behavior, see [`job_schema_gen` Design](job_schema_gen_design.md).
>
> For the end-to-end consumer example, see [`job_schema_example` Design](job_schema_gen_example_design.md).

---

# Purpose

`job_schema_gen.cmake` is the build-system integration layer for JOB Schema.

Its job is:

> **Given one or more C++ schema source files, create a normal generated C++ library target and arrange everything required to build it correctly.**

Conceptually:

```text id="55yy6q"
developer
    │
    ▼
job_schema_gen_library(...)
    │
    ├── schema source files
    ├── target metadata
    └── current toolchain/build context
    │
    ▼
job_schema_gen
    │
    ▼
generated C++
    │
    ▼
normal CMake library target
```

The CMake layer owns build orchestration.

It does not own schema meaning.

---

# Design Goal

The public developer experience should remain simple and familiar.

Conceptually:

```cmake id="q4zx4r"
job_schema_gen_library(
    NAME job_schema_example_generated
    VERSION 0.1
    DESCRIPTION "Generated JOB Schema example library"

    EXPORT_HEADER jobschemaexample_export.h
    EXPORT_MACRO JOBSCHEMAEXAMPLE_EXPORT

    SCHEMA_FILES
        input/example_req.h
        input/example_user.h
        input/example_tx.h
)
```

The developer should not need to manually write:

```text id="y1ctqd"
temporary compiler commands
helper targets
reflection flags
QEMU execution commands
generated source lists
temporary include paths
custom linker commands
```

Those are implementation details of the build integration.

---

# Relationship to the Legacy Generator

The public shape should intentionally feel familiar to the previous JOB code-generation flow.

Historically:

```cmake id="jgvcip"
job_msgpack_gen_lib(
    NAME ${TARGET_NAME}
    VERSION 0.1
    DESCRIPTION "use the message pack compiler too make a lib"

    EXPORT_HEADER jobmsggen_export.h
    EXPORT_MACRO JOBMSGGEN_EXPORT

    SCHEMA_FILES
        input/example_req.yaml
        input/example_user.yaml
        input/example_tx.json
)
```

JOB Schema should preserve the useful part of that experience:

```text id="xzuipu"
declare generated library
provide schema inputs
build normally
```

while replacing the old external-schema/parser architecture with:

```text id="3zdctn"
C++ schema
    ↓
target compiler
    ↓
C++26 reflection
    ↓
generated C++
```

The CMake function should therefore feel evolutionary even though the generator internals are completely different.

---

# Core Responsibilities

`job_schema_gen.cmake` owns:

```text id="bw4wcn"
public generation function
argument parsing
schema input registration
generator invocation
toolchain propagation
temporary build path selection
custom command dependencies
generated file registration
generated library creation
public include propagation
export-header integration
install/export integration where requested
```

It should make the generated result behave like an ordinary CMake library target.

---

# Non-Goals

## It Does Not Understand Schema Semantics

CMake must not decide:

```text id="xz8ne9"
Struct
Packed
BaseObject
LightObject
Object
RW
RO
Const
Required
```

Those meanings live in C++.

The build function should not require configuration such as:

```cmake id="d4ydfa"
TYPE OBJECT
```

or:

```cmake id="ti5c3h"
PACKED TRUE
```

when the schema declaration already says what it is.

The rule is:

> **CMake knows which files are schemas. C++ knows what those schemas mean.**

---

## It Is Not a C++ Parser

The CMake layer should never inspect source text to determine:

```text id="iv7cyy"
schema type name
member names
annotations
schema kind
nested type relationships
```

That belongs to the compiler/reflection path.

---

## It Does Not Invoke a Second Schema Language

There should be no build-generated:

```text id="pj22ja"
YAML description
JSON metadata
MessagePack schema
type-name list
schema IR file
```

merely to communicate between CMake and `job_schema_gen`.

The generator should receive normal build/toolchain inputs and C++ schema source paths.

---

## It Is Not a Parallel Build System

CMake/Ninja already own scheduling and dependency execution.

`job_schema_gen.cmake` should describe correct dependencies to the build system rather than creating its own scheduler.

---

# Public Build Shape

The preferred public abstraction is one function that produces one visible generated library target.

Conceptually:

```cmake id="9ix2g4"
job_schema_gen_library(
    NAME MyGeneratedLibrary
    SCHEMA_FILES
        foo.h
        bar.h
)
```

The result should be a normal target:

```cmake id="qjygq9"
target_link_libraries(my_app PRIVATE
    MyGeneratedLibrary
)
```

Consumers should not need to know whether the generated library required:

```text id="tk4l1m"
one helper
three helper compilations
response files
QEMU
temporary executables
staging directories
```

Those are internal build details.

---

# One Visible Target

Temporary generation machinery should not leak into the public target model.

The developer should see:

```text id="vj4apx"
MyGeneratedLibrary
```

not a forest of targets such as:

```text id="mrc0oc"
MyGeneratedLibrary_schema_compile
MyGeneratedLibrary_reflect
MyGeneratedLibrary_helper
MyGeneratedLibrary_codegen
MyGeneratedLibrary_stage
MyGeneratedLibrary_generated
```

Internal targets or custom commands may exist if implementation requires them.

They should remain implementation details.

The public abstraction is the generated library.

---

# Schema Inputs

The function accepts C++ schema files.

Conceptually:

```cmake id="knpdg3"
SCHEMA_FILES
    input/example_req.h
    input/example_user.h
    input/example_tx.h
```

These are build inputs.

The CMake layer does not need to know their schema kind.

It only needs to ensure they participate in the correct generation dependencies.

The exact schema-source structural rules belong in the schema design documentation rather than this file.

---

# Toolchain Propagation

`job_schema_gen` must compile temporary helper code using the effective target toolchain.

Therefore the CMake integration must propagate the relevant build context.

This may include:

```text id="b8hgfy"
CMAKE_CXX_COMPILER
CMAKE_CXX_STANDARD
CMAKE_SYSROOT
target architecture options
include directories
compile definitions
compile options
reflection flags
contract flags
link options
cross-compiling state
configured emulator
```

The exact transport mechanism is an implementation decision.

The requirement is:

> **The temporary schema compilation must see the same target-relevant environment required to interpret and generate the final library correctly.**

---

# Target Compiler Authority

The CMake layer must not substitute a convenient host compiler when the build is targeting another platform.

For cross compilation:

```text id="3ek95d"
host
    runs CMake and job_schema_gen

target compiler
    compiles schema helper
    determines ABI/reflection facts
```

This matters especially for:

```text id="zd2v15"
sizeof
alignof
Packed layout
target ABI
compiler-specific reflection behavior
```

The target compiler remains authoritative.

---

# C++ Standard and JOB Flags

The schema generation compilation must use the JOB-required C++ mode.

Conceptually:

```text id="2mb0pc"
C++26
reflection enabled
contracts enabled where required
```

For the current GCC development environment this may require flags such as:

```text id="1oduko"
-freflection
-fcontracts
```

The exact compiler flags are toolchain-specific implementation details.

The CMake integration should centralize them rather than requiring each generated library to repeat them manually.

---

# Compile Definitions and Include Paths

Schema files are real C++ and may depend on normal project headers.

Therefore temporary schema compilation must receive relevant:

```text id="u9oae0"
include directories
compile definitions
system include paths
generated include paths where appropriate
```

The CMake layer should derive these from the generated target and explicitly supplied dependencies rather than forcing developers to duplicate the same information in schema-specific arguments wherever possible.

The guiding rule is:

> **If the generated library already needs a build property, schema compilation should normally inherit the same relevant property.**

---

# Dependencies

The generated library may depend on normal JOB or application libraries.

Conceptually the public function may support something equivalent to:

```cmake id="ez2c9i"
DEPENDS
    JosephsOddBuilder_Core
    SomeOtherLibrary
```

The final syntax remains open.

Those dependencies may serve two purposes:

```text id="krt4af"
provide includes/definitions required during schema compilation
link the final generated library where required
```

The implementation must distinguish build requirements from final link requirements where necessary.

It should not blindly pass every property everywhere.

---

# Generator Invocation

The CMake layer invokes `job_schema_gen`.

Conceptually:

```text id="kml856"
custom command
    │
    ├── schema inputs
    ├── generator executable
    ├── compiler/toolchain configuration
    ├── output location
    └── dependency information
    │
    ▼
generated .h / .cpp
```

The exact command-line format is owned by implementation.

It may use:

```text id="dssxp7"
direct arguments
response files
generated configuration files
```

as appropriate.

CMake should favor robust argument passing over shell-string construction.

---

# Response Files

Real compiler invocations may have long argument lists.

A response-file mechanism may be useful for carrying:

```text id="z5jg1u"
include paths
compile definitions
compiler options
sysroot options
target flags
```

without command-line length and quoting problems.

This is an implementation option, not a public requirement.

The public CMake interface should not expose response-file mechanics.

---

# Generated Output Directory

Generated files should live in the build tree.

Conceptually:

```text id="xo66ek"
<binary-dir>/generated/<target>/
```

or another deterministic target-specific location.

The exact path may change.

The requirements are:

```text id="x6rx5k"
not written into source tree
target-specific
parallel-build safe
available as public include directory
predictable for build dependencies
```

---

# Temporary Generation Directory

Compiler/helper scratch files should also remain in the build tree or another build-owned location.

Conceptually:

```text id="jfechn"
<binary-dir>/.job_schema/<target>/
```

These may include:

```text id="a3dx74"
temporary TU
response files
object files
helper executable
logs
staging output
```

They are not public generated-library sources.

---

# Generated Files

The CMake integration must register generated source/header files correctly so the normal build system understands that they do not exist until generation runs.

Generated sources should be marked appropriately for CMake/Ninja.

The final library target should compile those files normally after generation.

Conceptually:

```text id="30krgj"
schema input
    ↓
custom generation command
    ↓
generated .h/.cpp
    ↓
add_library(...)
```

---

# Generated Library

After generation, the function creates a normal C++ library.

Conceptually:

```cmake id="pupcsh"
add_library(MyGeneratedLibrary
    ${GENERATED_HEADERS}
    ${GENERATED_SOURCES}
)
```

The library should then receive normal CMake properties such as:

```text id="2ux93v"
VERSION
SOVERSION
CXX_STANDARD
include directories
link dependencies
export visibility
install/export rules
```

The generated target should behave like a hand-written library as far as consumers are concerned.

---

# Export Header

When building a shared generated library, the function may generate or configure a normal export header.

Conceptually:

```cmake id="zrqvml"
EXPORT_HEADER mygenerated_export.h
EXPORT_MACRO MYGENERATED_EXPORT
```

This mirrors existing JOB generated-library patterns.

The schema itself should not know or care about export macros.

Visibility is a generated-library/build concern.

---

# Public Include Directory

Consumers should receive the generated include path through the generated library target.

They should be able to write ordinary code such as:

```cpp id="p33454"
#include <example_req.h>
```

without manually adding generator staging paths.

Conceptually the generated target should provide:

```cmake id="tt6x4y"
target_include_directories(MyGeneratedLibrary PUBLIC
    <generated include directory>
)
```

The exact path layout remains implementation policy.

---

# Native Builds

On a native build the flow is conceptually:

```text id="fcu1o5"
CMake
   ↓
job_schema_gen
   ↓
target/native compiler
   ↓
generation helper
   ↓
run directly
   ↓
generated source
   ↓
generated library
```

No special developer configuration should be required beyond the normal toolchain.

---

# Cross Builds

On a cross build:

```text id="wxtzgd"
CMake
   ↓
job_schema_gen
   ↓
cross compiler
   ↓
target helper
   ↓
configured emulator
   ↓
generated source
   ↓
cross-compiled generated library
```

The CMake integration should propagate the configured execution mechanism.

Where available, this should naturally integrate with:

```text id="scxyt4"
CMAKE_CROSSCOMPILING
CMAKE_CROSSCOMPILING_EMULATOR
```

or equivalent project/toolchain configuration.

The build function should not hard-code QEMU paths.

---

# QEMU Is an Execution Detail

The CMake layer may arrange for QEMU or another emulator to execute the target helper.

It must not treat the emulator as the source of:

```text id="wbnap3"
ABI information
reflection information
layout information
schema validation
```

Those come from the compiler.

The emulator only runs the resulting target-side helper when required.

---

# Incremental Builds

The CMake integration must describe dependencies accurately enough that generation reruns when meaningful inputs change.

These may include:

```text id="o3je46"
schema source
transitive schema includes
job_schema headers
job_schema_gen executable
generation support headers
compiler configuration
relevant target properties
```

Where useful, compiler-generated dependency files may be used to expose transitive header dependencies to Ninja.

The exact depfile strategy is an implementation detail.

---

# Avoiding Unnecessary Rebuilds

The build integration should cooperate with generator behavior that avoids rewriting unchanged files.

If generated output has not semantically changed, downstream recompilation should be avoided where practical.

This matters because generated libraries may sit low in a larger build dependency tree.

---

# Parallel Builds

Multiple generated libraries may build at the same time.

Therefore all internal paths must be scoped sufficiently to avoid collisions.

The implementation must not assume:

```text id="in11ve"
one global helper filename
one global response file
one global temp directory
one global generated output directory
```

Parallel Ninja builds are a normal expected use case.

---

# Configure Time Versus Build Time

Schema generation should normally happen at build time rather than executing the full generator during CMake configure.

CMake configure should establish:

```text id="hknmx0"
targets
custom commands
dependencies
output paths
```

The compiler/reflection/helper execution belongs in the normal build graph.

This preserves proper incremental behavior and avoids making CMake configuration perform large amounts of compilation work.

---

# Failure Propagation

Any failure in generation must fail the CMake build.

Relevant stages include:

```text id="tsjpxz"
schema compile
helper link
helper execution
configured emulator execution
file generation
generated-source compilation
```

The underlying diagnostic should remain visible.

The CMake layer should not replace useful compiler/generator output with generic messages.

---

# Installation

If the generated library is configured for installation, the function should be able to install:

```text id="gbyzx7"
generated library
generated public headers
CMake export target
package metadata where applicable
```

using the same conventions as normal JOB libraries.

Temporary schema-helper artifacts must never be installed.

Schema input files are build inputs and should not automatically become installed public artifacts unless some separate design explicitly requires that behavior.

---

# Generated Target Export

The generated library should be exportable through normal CMake mechanisms.

Conceptually:

```cmake id="nbbj1y"
install(TARGETS MyGeneratedLibrary
    EXPORT MyGeneratedLibraryTargets
    ...
)
```

The exact namespace and package configuration conventions should follow existing JOB library practices.

---

# Source Tree Cleanliness

The CMake integration must not require checked-in generated C++.

A clean repository should contain:

```text id="1asc8p"
schema source
normal project source
CMake files
```

while the build tree contains:

```text id="829n77"
temporary generation artifacts
generated headers
generated sources
compiled generated library
```

This clean separation is part of the example and CI verification strategy.

---

# Product Requirements

The implementation of `cmake/job_schema_gen.cmake` should satisfy the following requirements.

1. It must provide one public function for creating a generated JOB Schema library.

2. The public function must accept C++ schema source files.

3. It must not require CMake to specify schema kind or member policy.

4. It must not parse schema C++.

5. It must not create or require a second schema language or serialized schema IR.

6. It must invoke `job_schema_gen` as part of the normal build graph.

7. It must propagate the configured target compiler environment required by schema generation.

8. It must support C++26 reflection/compiler flags required by JOB Schema.

9. It must propagate relevant include directories and compile definitions.

10. It must support target/build dependencies required by schema compilation and generated-library linking.

11. It must create target-specific temporary and generated output paths.

12. It must support parallel builds safely.

13. Generated C++ must live in the build tree.

14. Generated files must be correctly registered as build outputs.

15. The generated library must behave like a normal CMake library target.

16. Consumers must receive generated public include directories through the target.

17. Shared-library generation must support normal JOB export-header conventions.

18. Native helper execution must work transparently.

19. Cross-target helper execution must work through the configured emulator where required.

20. QEMU/emulator paths must not be hard-coded into the public design.

21. Generation failures must fail the build and preserve useful underlying diagnostics.

22. Incremental builds must rerun generation when relevant schema/generator/toolchain inputs change.

23. Unchanged generated output should not force unnecessary downstream rebuilds where practical.

24. Temporary generation artifacts must remain internal build products.

25. Generated libraries should support normal JOB installation/export behavior.

26. Public consumers must not need to know internal generator/helper paths.

27. CMake must remain build orchestration only; schema semantics stay in C++.

---

# Decisions Already Made

The following are current design decisions:

```text id="tuhwxr"
job_schema_gen.cmake owns build orchestration.

C++ owns schema semantics.

The public abstraction is one generated library target.

Schema files are C++ build inputs.

Schema kind is not repeated in CMake.

Member policy is not repeated in CMake.

The target compiler is used for schema generation.

Relevant toolchain state is propagated into generation.

Generated source lives in the build tree.

Temporary helper artifacts remain private.

Native generation runs directly.

Cross generation uses the configured emulator when required.

QEMU is not hard-coded as the schema architecture.

Generated libraries behave like normal CMake libraries.

The public CMake shape should remain familiar to existing JOB generated-library workflows.

Generated source is not checked into the source tree.
```

---

# Open Design Questions

## Exact Function Name

The conceptual public function is currently:

```cmake id="j7lg52"
job_schema_gen_library(...)
```

The exact final spelling should remain consistent with existing JOB CMake naming conventions.

---

## Target Naming

Should:

```cmake id="4p23df"
NAME Foo
```

produce exactly:

```text id="3njpqn"
Foo
```

or should the function apply an internal generated-target naming convention?

The simplest normal-CMake behavior is preferable unless a concrete collision problem requires otherwise.

---

## Dependency Argument

What public argument best represents libraries required by generated schema compilation?

Possible naming includes:

```text id="sjflst"
DEPENDS
LIBRARIES
LINK_LIBRARIES
```

The implementation should avoid inventing several overlapping dependency lists unless they are genuinely needed.

---

## Compiler Configuration Transport

Should CMake pass compiler/toolchain state through:

```text id="klh9ad"
direct command-line arguments
response file
generated configuration file
```

?

Response files are attractive for large real-world compile environments.

This should be decided through implementation rather than over-designed in advance.

---

## Output Naming

How should input schema filenames map to generated:

```text id="neq027"
header names
source names
```

?

The rule should be deterministic and unsurprising.

Exact schema/output naming belongs partly to the schema-input conventions defined elsewhere.

---

## Dependency Files

Should the temporary schema compilation emit compiler depfiles so Ninja automatically tracks transitive schema includes?

This is likely useful and should be tested early.

---

## Installation Defaults

Should generated libraries install automatically when version/export metadata are provided, or should installation remain explicitly requested?

This should follow normal JOB CMake conventions rather than creating special Schema behavior.

---

# Design Principles

The CMake integration should repeatedly return to a few rules:

> **CMake knows how to build it. C++ knows what it is.**

> **Do not repeat schema semantics in build configuration.**

> **Do not expose temporary generation machinery as public targets.**

> **Do not make developers manually reconstruct the compiler environment for schema generation.**

> **Do not write generated source into the source tree.**

> **Do not make generated libraries feel different from normal libraries after generation is complete.**

The desired public experience is:

```text id="wop02v"
write C++ schemas
      ↓
list them in job_schema_gen_library(...)
      ↓
target_link_libraries(...)
      ↓
build normally
```

Everything underneath that belongs to the build integration.

That is CMake's one job.
