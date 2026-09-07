# JOB Schema — High-Level Architecture

JOB Schema makes a C++ schema declaration the single source of truth.

A member is declared once, and the repetitive code around that member—accessors, signals, persistence behavior, reset behavior, validation hooks, and physical layout where applicable—is derived from that declaration.

The goal is simple:

> **A field added in one place should not be able to silently go missing from several others.**

JOB Schema uses real C++26 structures as schema input. The C++ compiler provides parsing, type checking, target ABI knowledge, and compile-time reflection. `job_schema` adds JOB-specific schema rules and compile-time validation, while `job_schema_gen` turns valid reflected declarations into generated C++.

There is no secondary YAML/JSON schema language and no intermediate schema IR between reflection and generation.

---

# System Overview

```text
┌──────────────────────────────────────────────────────────────────────────┐
│                           JOB SCHEMA PIPELINE                            │
└──────────────────────────────────────────────────────────────────────────┘

              Developer writes real C++ "schema structs"
                               │
                               ▼
                     apps/job_schema_example
                  (simple end-to-end application)
                               │
                               │ uses
                               ▼
                   cmake/job_schema_gen.cmake
                               │
                               │ orchestrates
                               ▼
                      apps/job_schema_gen
                               │
                    prepares compilation
                               │
                               ▼
                     TARGET C++ COMPILER
                      C++26 reflection
                     concepts / assertions
                      JOB schema linting
                               │
                      ┌────────┴────────┐
                      │                 │
                   invalid            valid
                      │                 │
                      ▼                 ▼
                COMPILE ERROR    generation helper
                                        │
                          ┌─────────────┴─────────────┐
                          │                           │
                     native build                cross build
                          │                           │
                     run directly               run via QEMU
                          │                           │
                          └─────────────┬─────────────┘
                                        │
                                        ▼
                                generated C++
                                        │
                                        ▼
                              TARGET C++ COMPILER
                                        │
                                        ▼
                             final generated library
                                        │
                 ┌──────────┬───────────┼───────────┬──────────┐
                 │          │           │           │          │
                 ▼          ▼           ▼           ▼          ▼
              Struct      Packed    BaseObject   LightObject  Object
```

The target compiler is the authority for:

```text
C++ validity
type correctness
concept satisfaction
schema assertions
target ABI
sizeof / alignof
target-specific layout
compile-time reflection
```

QEMU does **not** provide schema validation or target ABI information.

Those facts already exist after the target compilation step.

When cross-compiling, QEMU is used only when the resulting target-side generation helper must execute on the build host in order to materialize the generated C++ files.

---

# The Six Pieces

## 1. `libs/job_schema`

**Job:** define what a valid JOB schema means.

`job_schema` is the core backing library for the schema system. Most of its behavior is compile-time and reflection-driven.

It owns:

```text
schema kinds
schema annotations
reflection helpers
member-policy rules
schema concepts
compile-time validation
generation-facing reflection utilities
```

It understands the five schema kinds:

```text
Struct
Packed
BaseObject
LightObject
Object
```

It does **not** generate files itself.

It also does **not** introduce another schema representation between the reflected C++ declaration and the generator.

There is no:

```text
SchemaDescription
SchemaTypeDescription
SchemaMemberDescription
serialized schema IR
YAML intermediate
JSON intermediate
```

unless some future implementation detail proves genuinely necessary.

The reflected C++ type remains authoritative.

`job_schema` builds on top of `job_core`:

```text
job_core
   ↑
job_schema
```

---

## 2. `tests/job_schema`

**Job:** prove the schema system and document how it is used.

JOB tests serve both as verification and documentation.

`tests/job_schema` should follow the normal JOB three-block testing structure.

### Block One — Usage / Examples

These tests answer:

> How do I use this?

They should contain small, realistic schema declarations and demonstrate normal use of:

```text
Struct
Packed
BaseObject
LightObject
Object
```

along with their common annotations and generated behavior.

These tests are the primary developer examples.

There is intentionally no separate `examples/` hierarchy for this.

---

### Block Two — Edge Cases

These test invalid or unusual schema combinations and protect the generator's invariants.

Examples include:

```text
invalid annotation combinations
unsupported Packed members
missing Required fields
empty structures
nested structures
arrays
alignment edge cases
unsupported member types
invalid object policies
cross-type dependencies
```

Compile-time failure tests are especially important because the compiler itself is part of the schema linter.

A bad schema should fail during compilation rather than survive until some later generator-specific validation pass.

---

### Block Three — Benchmarks / Stress When Needed

When useful and enabled through:

```text
JOB_TEST_BENCHMARKS
```

schema tests can benchmark or stress:

```text
reflection traversal
generation
large object graphs
Packed layouts
serialization of generated BaseObject/Object types
signal-heavy generated objects
cross-target behavior
```

Not every test requires a benchmark section.

The testing path can naturally extend through:

```text
host tests
    ↓
cross-compiled tests
    ↓
QEMU tests
    ↓
real-device tests
```

The schema-generation system therefore becomes part of JOB's normal portability testing rather than a separate toolchain island.

---

## 3. `apps/job_schema_gen`

**Job:** turn valid reflected schema declarations into generated C++.

`job_schema_gen` is the generator application.

It uses:

```text
job_schema
job_io
```

and other JOB libraries where needed for:

```text
temporary files/directories
process execution
compiler invocation
linker invocation
QEMU/emulator execution
output generation
logging
error reporting
```

Its responsibilities are roughly:

```text
receive schema inputs
prepare temporary compilation
invoke the configured compiler/linker
run the target-side generation helper
emit generated headers/sources
report compilation/generation failures
```

The temporary schema compilation is also the lint pass.

If the schema contains:

```text
invalid C++
an invalid annotation
an unsupported member type
a failed concept
a failed static assertion
a target-layout violation
```

then compilation fails and generation stops.

`job_schema_gen` does **not** own a second implementation of the schema rules.

Its responsibility is:

> **Given a schema accepted by the compiler and `job_schema`, what C++ should be emitted?**

Reflection data is consumed directly as part of that generation process.

There is no serialized schema-description boundary between reflection and code emission.

---

## 4. `cmake/job_schema_gen.cmake`

**Job:** make the entire generation pipeline behave like a normal CMake library operation.

A developer should be able to write something conceptually like:

```cmake
job_schema_gen_library(
    NAME Foo

    SCHEMA_FILES
        foo.h
        bar.h
)
```

and receive one useful final target:

```text
Foo
```

Conceptually:

```text
schema headers
    ↓
job_schema_gen
    ↓
temporary target compilation
    ↓
C++ type checking
C++26 reflection
JOB schema linting
target ABI validation
    ↓
generation helper execution
    ↓
generated .h/.cpp
    ↓
normal target compilation
    ↓
Foo
```

The preferred public model is one generated library target.

Internal implementation details may involve:

```text
temporary source files
temporary objects
temporary executables
temporary shared objects
response files
compiler commands
linker commands
QEMU invocation
```

but these are build artifacts, not public products of the schema library.

The important ownership rule is:

> **CMake orchestrates. It does not understand schema semantics.**

CMake should not contain:

```text
RW / RO rules
Packed-member validation
Required semantics
Object-generation rules
reflection rules
persistence rules
```

Those belong elsewhere.

CMake owns:

```text
compiler selection
toolchain information
cross-compilation state
sysroot
include paths
compile definitions
dependency propagation
emulator configuration
build dependencies
output locations
final library construction
```

---

## 5. `apps/job_schema_example`

**Job:** demonstrate the complete schema-generation pipeline as a real application.

`job_schema_example` is intentionally under `apps/`.

Even though its primary purpose is demonstrating generation of a library, it is still a real executable application that consumes that generated library.

Detailed usage examples belong in `tests/job_schema`.

`job_schema_example` demonstrates the end-to-end build-system story:

```text
write schema structs
    ↓
call job_schema_gen.cmake
    ↓
compile/lint the schema
    ↓
generate C++
    ↓
compile generated library
    ↓
link and use generated library
```

It should contain a deliberately small set of schema declarations covering the five output modes:

```text
apps/job_schema_example/
├── CMakeLists.txt
├── schema/
│   ├── plain_struct.h
│   ├── packed_struct.h
│   ├── base_object.h
│   ├── light_object.h
│   └── object.h
└── main.cpp
```

Its CMake configuration invokes:

```cmake
job_schema_gen_library(...)
```

to create a generated library.

Another application technically **could** link against that generated library, but that is not its purpose.

The application exists to prove that the entire pipeline works exactly as a real consumer would use it.

---

## 6. `job_core`

`job_core` is not a new schema component, but it is the runtime foundation underneath the entire system.

It already owns:

```text
BaseObject
LightObject
Object
Signal
Connection
reflection-based serialization
JSON
YAML
binary serialization
reset behavior
NoSerialize
NoReset
contracts/runtime invariant support
logging/error integration
```

`job_schema` consumes and builds on these facilities rather than reimplementing them.

This is particularly important for:

```text
BaseObject
LightObject
Object
```

The generator must emit code that follows the existing `job_core` semantics.

---

# The Five Output Modes

The schema kind determines the representation `job_schema_gen` produces.

## `Struct`

Plain C++.

The schema declaration is already the desired representation.

No generated:

```text
factories
pointer aliases
accessors
signals
special lifetime policy
persistence behavior
```

The header can effectively pass through as normal C++.

---

## `Packed`

A fixed-layout, memcpy-friendly representation.

The developer describes the logical fields.

The generator may:

```text
reorder members
insert explicit padding
reduce wasted space
produce fixed-size layout
emit compile-time layout assertions
```

without requiring the developer to manually perform those layout calculations.

`Packed` is intended for representations such as:

```text
memcpy
mmap
shared memory
compact binary records
fixed transport records
```

It deliberately does not generate:

```text
Rule-of-Five machinery
pointer factories
signals
property accessors
behavioral object semantics
```

The target compiler remains the final authority for the resulting physical layout.

---

## `BaseObject`

A persistence-oriented JOB object.

The generated type derives from `BaseObject` and receives the normal JOB object support appropriate to that family, including:

```text
Ptr / WPtr / UPtr aliases
createShared()
createUniq()
generated member API
persistence integration
reset integration
```

Its copy/move/lifetime policy should follow the actual intended `BaseObject` semantics rather than blindly generating boilerplate merely because a "Rule of Five" checklist exists.

The exact generated special-member policy belongs in the detailed `BaseObject` design, not page one.

---

## `LightObject`

A runtime JOB object without `BaseObject` persistence.

The generated type derives from `LightObject` and receives:

```text
Ptr / WPtr / UPtr aliases
createShared()
createUniq()
generated member API
Signal integration
connection behavior
explicit JOB lifetime/copy/move policy
```

`LightObject` remains a separate hierarchy from `BaseObject`.

It does not inherit persistence simply because `Object` does.

---

## `Object`

The full JOB runtime + persistence object.

The generated type derives from `Object`.

`Object` combines:

```text
BaseObject persistence
runtime identity
signals
connections
generated property API
JOB lifetime/copy/move policy
pointer aliases/factories
```

This is the largest generated object family.

The actual runtime hierarchy is:

```text
BaseObject
    ▲
    │
  Object


LightObject
    separate hierarchy
```

The five schema kinds are generator modes and should therefore normally be shown flat rather than drawn as if they all form one inheritance tree.

---

# `isValid()` Is Not Part of the Schema Model

`Object` and `LightObject` should no longer require a pure virtual:

```cpp
bool isValid() const noexcept;
```

and the corresponding concepts should not require it either.

A universal `isValid()` query mixes together several unrelated kinds of validation.

JOB now has clearer mechanisms for those concerns.

### Programmer and runtime invariants

Use C++ contracts:

```text
preconditions
postconditions
runtime invariants
programmer errors
```

A contract violation means code has violated an expected invariant.

---

### Schema/input presence

Use schema policy such as:

```text
Required
```

This is handled while the input representation still contains presence information.

---

### User-facing/domain validation

This remains normal application validation.

Examples include:

```text
manifest semantic validation
multiple human-readable errors
field paths
invalid combinations
configuration diagnostics
```

Bad user input is not a contract violation.

Contracts do not replace application-level validation.

Removing `isValid()` therefore does **not** mean:

```text
all validation → contracts
```

It means each kind of validation is handled by the mechanism that actually has the information needed to perform it.

`Object` and `LightObject` may remain abstract by design without inventing a meaningless pure virtual validation function solely to force abstractness.

The exact mechanism for preserving that abstract-base behavior belongs in their detailed design.

---

# Member Policies and Annotation Ownership

Annotations should describe behavior that cannot simply be inferred from the C++ member declaration itself.

They should not duplicate facts reflection already knows.

Reflection already supplies information such as:

```text
member name
member type
member order
array extent
inheritance
annotations
size/alignment where applicable
```

The policies split naturally into separate groups.

---

## Generated API Policy — `job_schema`

```text
RW
RO
Const
```

These control what public/protected API is emitted around a member.

Depending on schema kind, they may affect:

```text
getter generation
setter generation
setter visibility
reset generation
changed Signal generation
```

### `RW`

The generated API allows reading and writing.

### `RO`

The generated API allows external reading but not normal external writing.

The containing class may still receive an internal/protected setter where appropriate.

`RO` therefore means:

> externally read-only

not:

> permanently immutable

### `Const`

`Const` is a stronger generated API restriction.

It can suppress things such as:

```text
setter
reset API
changed Signal
```

depending on the final policy table.

However:

> **`Const` constrains the generated API only. It does not make the underlying persisted member immutable.**

Reflection-based reconstruction such as:

```text
fromJson
fromYaml
fromBinary
```

may still populate that field when restoring an object.

If true immutable storage is ever needed, that is a separate concept.

---

# Input Presence Policy — `job_schema`

```text
Required
```

`Required` means:

> **This field must be present in an input representation that can distinguish present fields from absent fields.**

It does not mean:

```text
integer != 0
string != ""
value != default initializer
```

For example:

```json
{}
```

may fail for a required integer field.

But:

```json
{
    "count": 0
}
```

satisfies the presence requirement because the field was explicitly supplied.

This definition works consistently for every member type.

Presence can only be enforced while reading an input representation because that is where the distinction between:

```text
missing
```

and:

```text
present with the default value
```

still exists.

Therefore:

> **`Required` is defined by `job_schema`, but persistence readers in `job_core` enforce it.**

For example, the current pattern:

```cpp
if (j.contains(key)) {
    // deserialize member
}
```

will need to recognize reflected `Required` annotations and record missing members.

Missing required fields should be accumulated during parsing so the caller can receive all relevant failures in one operation rather than discovering them one at a time.

How those diagnostics are ultimately formatted and surfaced can use the existing JOB logging/error infrastructure.

`Required` is presence validation, not generalized domain validation.

---

# Existing Persistence / Reset Policy — `job_core`

```text
NoSerialize
NoReset
```

These annotations already exist in `job_core`.

`job_schema` does not redefine them.

The generator sees them through reflection and honors their existing semantics when generating `BaseObject` and `Object` types.

The ownership model is therefore:

```text
job_schema
    defines schema-level API policy
    defines schema-level presence policy

job_core
    owns persistence/reset behavior
    enforces persistence-related schema policy during parsing
```

This is intentional.

The library that owns the operation should enforce the relevant policy while performing that operation.

---

# Validation Model

JOB Schema should keep three different classes of validation separate.

```text
┌──────────────────────────────────────────────────────────────┐
│                    INPUT / PRESENCE                          │
│                                                              │
│  Required                                                    │
│  missing fields                                              │
│  malformed serialized input                                  │
│                                                              │
│  handled while parsing                                       │
└──────────────────────────────────────────────────────────────┘

                            │

┌──────────────────────────────────────────────────────────────┐
│                    DOMAIN VALIDATION                         │
│                                                              │
│  semantic correctness                                        │
│  field-path diagnostics                                      │
│  multiple human-readable errors                              │
│  application-specific rules                                  │
│                                                              │
│  handled by application/domain code                          │
└──────────────────────────────────────────────────────────────┘

                            │

┌──────────────────────────────────────────────────────────────┐
│                    PROGRAM INVARIANTS                        │
│                                                              │
│  preconditions                                               │
│  postconditions                                              │
│  programmer/runtime invariants                               │
│                                                              │
│  handled with C++ contracts                                  │
└──────────────────────────────────────────────────────────────┘
```

These mechanisms complement one another.

They should not be collapsed into a universal `isValid()` function.

---

# The Core Build Flow

```text
                  REAL C++ SCHEMA STRUCTS
                            │
                            ▼
                  job_schema_gen.cmake
                            │
                            ▼
                     job_schema_gen
                            │
                            ▼
                   TARGET C++ COMPILER
                            │
                 ┌──────────┴──────────┐
                 │                     │
                 ▼                     ▼
          normal C++ checks       libs/job_schema
          target ABI              schema concepts
          C++26 reflection        annotations
          type checking           validation rules
                 │                     │
                 └──────────┬──────────┘
                            │
                   ┌────────┴────────┐
                   │                 │
                invalid            valid
                   │                 │
                   ▼                 ▼
             COMPILE ERROR     generation helper
                                      │
                           ┌──────────┴──────────┐
                           │                     │
                        native                cross
                           │                     │
                      run directly          run via QEMU
                           │                     │
                           └──────────┬──────────┘
                                      │
                                      ▼
                               GENERATED C++
                                      │
                                      ▼
                            TARGET C++ COMPILER
                                      │
                                      ▼
                            GENERATED LIBRARY
```

The first compilation proves the schema declaration.

It establishes:

```text
valid C++
valid types
valid annotations
valid concepts
valid JOB schema rules
target ABI facts
```

When cross-compiling, these facts still come from the target compiler.

QEMU only provides an execution environment for the target-side helper when that helper must run on a different host architecture.

The second compilation proves the emitted C++ and produces the actual generated library.

This gives two complementary compiler gates:

```text
schema compilation
    proves the source schema

generated compilation
    proves the generated implementation
```

The resulting library then enters the normal JOB testing stack.

---

# High-Level Ownership

```text
job_core
    runtime object model
    serialization
    signals/connections
    persistence/reset annotations
    parsing enforcement
    contracts
    logging/error support

        │
        ▼

libs/job_schema
    schema vocabulary
    schema kinds
    API annotations
    presence annotations
    reflection helpers
    concepts
    compile-time schema rules

        │
        ▼

apps/job_schema_gen
    compiler/generator application
    invokes target compilation
    runs generation helper
    emits C++

        │
        ▼

cmake/job_schema_gen.cmake
    build orchestration
    toolchain wiring
    cross-compilation wiring
    generated-library construction

        │
        ▼

generated library
    Struct
    Packed
    BaseObject
    LightObject
    Object

        │
        ├──────────────► tests/job_schema
        │                 usage / documentation
        │                 edge cases
        │                 benchmarks / stress
        │
        └──────────────► apps/job_schema_example
                          complete build/use demonstration
```

---

# Architectural Rules

The page-one rules are:

> **The C++ schema declaration is the single source of truth.**

> **A field is declared once; repetitive implementation is derived from it.**

> **The target compiler is the parser, type checker, ABI authority, reflection engine, and first-stage linter.**

> **`job_schema` defines JOB-specific schema policy and compile-time rules.**

> **`job_core` enforces schema policies that belong to operations it already owns, such as persistence.**

> **`job_schema_gen` emits code directly from reflected C++ declarations and does not invent another schema language or serialized intermediate representation.**

> **CMake wires the build together; it does not implement schema semantics.**

> **`Required` means input presence, not value validity.**

> **Contracts protect programmer/runtime invariants; they do not replace human-facing domain validation.**

> **`Object` and `LightObject` do not require a universal `isValid()` function.**

> **Generated code is compiled normally and then enters the same host, cross-build, QEMU, and real-device testing stack as the rest of JOB.**


## Design Documents

- [job_schema](job_schema_design.md) — Core schema vocabulary, reflection, annotations, and validation.
- [job_schema_gen](job_schema_gen_design.md) — Compiler/generator application and code emission.
- [job_schema_example](job_schema_gen_example_design.md) — End-to-end generated-library example.
- [tests/job_schema](test_job_schema_design.md) — Schema testing strategy.
- [Dependencies and Build Integration](job_schema_design_deps.md) — CMake, toolchain, cross-compilation, and library dependencies.
