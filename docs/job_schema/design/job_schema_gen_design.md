# `job_schema_gen` Design

> **Document purpose:** This is a design and implementation-planning document for `apps/job_schema_gen`.
>
> It records the requirements, architectural boundaries, compiler-driver behavior, compile-time generation model, cross-compilation requirements, diagnostics, and open implementation questions for the JOB Schema generator.
>
> It is not API reference documentation and should not be treated as a promise of exact command-line options, class names, helper names, generated file names, or internal emitter implementation details.
>
> For the overall pipeline, see [JOB Schema — High-Level Architecture](intro.md).
>
> For schema semantics, annotations, and compile-time validation rules, see [`job_schema` Design](job_schema_design.md).
>
> For build-system orchestration and dependency wiring, see [JOB Schema Dependencies and Build Integration](job_schema_design_deps.md).
>
> For process and filesystem facilities used by the generator, see [JOB IO](../../job/io_overview.md).

---

# Purpose

`job_schema_gen` is the application responsible for turning valid C++26 schema declarations into generated C++.

Its input is not a secondary schema language.

Its input is real C++.

The generator therefore uses the actual C++ compiler and C++26 reflection implementation to understand schema declarations rather than attempting to parse or independently model C++ itself.

At the highest level:

```text id="xwtw6x"
schema source
    ↓
target C++ compiler
    ↓
C++26 reflection
    ↓
JOB schema validation
    ↓
constant-evaluation generation
    ↓
compiled generation helper
    ↓
write generated C++
```

The primary design goal is:

> **Use the compiler to understand C++, use `job_schema` to define what the schema means, and use `job_schema_gen` only to turn those facts into ordinary generated C++.**

---

# Why `job_schema_gen` Exists

The generated representations:

```text id="yboaa9"
Packed
BaseObject
LightObject
Object
```

may contain considerably more implementation detail than the declaration that describes them.

For example:

```cpp id="7is814"
[[=job::schema::Object{}]]
struct Device {
    [[=job::schema::RW{}]]
    std::string name{};

    [[=job::schema::RO{}]]
    std::uint32_t state{};
};
```

may ultimately require ordinary C++ containing:

```text id="4uj2w7"
inheritance
backing members
getters
setters
Signals
reset behavior
persistence-compatible state
JOB pointer aliases
factory helpers
copy/move/lifetime policy
includes
namespaces
contracts where mechanically derivable
```

That repetitive transformation should be implemented once.

The generator does not exist to replace C++.

It exists to remove C++ that humans should not have to maintain by hand.

---

# Core Responsibilities

`job_schema_gen` owns the mechanics required to transform an accepted schema declaration into generated source.

Its responsibilities include:

```text id="rdjqnk"
prepare schema compilation
invoke the configured compiler
provide generation support to the temporary compilation
surface compiler diagnostics
link a generation helper where required
execute that helper
support native and cross-target execution
write generated files
manage temporary generation artifacts
report generation-stage failures
```

It consumes semantic rules from `job_schema`.

It does not redefine those rules.

The ownership split is:

```text id="dof15j"
job_schema
    what a schema means
    what is valid

job_schema_gen
    how valid reflected declarations become generated C++

job_schema_gen.cmake
    how the build invokes and consumes generation
```

---

# Non-Goals

## It Is Not a C++ Parser

`job_schema_gen` must not independently parse C++ source.

It should not implement another parser for:

```text id="dq277f"
namespaces
templates
types
declarations
inheritance
initializers
annotations
```

The intended path is:

```text id="hu297j"
C++
 ↓
compiler
 ↓
C++26 reflection
 ↓
job_schema
 ↓
job_schema_gen
```

not:

```text id="pd8i9u"
header text
 ↓
custom parser
 ↓
approximation of C++
```

---

## It Is Not a Second Schema Validator

`job_schema_gen` does not own a parallel implementation of rules for:

```text id="z35ieu"
RW
RO
Const
Required
Packed legality
BaseObject semantics
LightObject semantics
Object semantics
annotation combinations
```

Those rules belong to the appropriate schema/core layer.

The schema compilation itself is the first lint stage.

If a declaration violates a compile-time schema rule, compilation should fail before source generation completes.

The generator may still report:

```text id="n63klr"
compiler failures
linker failures
helper execution failures
filesystem failures
internal impossible states
```

but it should not reproduce the schema-policy engine procedurally.

---

## It Is Not an Intermediate Schema Language

There is no required architecture such as:

```text id="oujk8r"
C++ reflection
    ↓
SchemaDescription
    ↓
SchemaMemberDescription
    ↓
serialize schema IR
    ↓
generator
```

The C++ declaration remains authoritative.

Generation may use small temporary compile-time structures when needed to perform a specific calculation.

For example, Packed generation may internally need information equivalent to:

```text id="u69ufn"
member size
member alignment
calculated offset
padding
```

That does not create a second schema language.

The architectural line is:

> **Temporary implementation state may exist, but there is no general-purpose secondary schema representation forming the boundary between reflection and generation.**

In particular there is no required:

```text id="dxjubs"
YAML schema IR
JSON schema IR
MessagePack schema IR
SchemaDescription persistence
public schema database
```

The generator should remain as close to reflected C++ as practical.

---

## It Is Not the Build System

`job_schema_gen` does not decide:

```text id="m2uoej"
public CMake target structure
installation policy
export sets
consumer linkage
project-level build dependencies
```

Those remain CMake responsibilities.

The generator receives the toolchain and output information required to perform its own job.

It does not replace the project build graph.

---

# Compiler-Driven Generation

The target C++ compiler is part of the generator architecture.

The first compilation proves:

```text id="bbjywp"
valid C++
valid includes
valid types
valid templates
valid annotations
valid concepts
valid JOB schema rules
target ABI facts
required reflection support
```

The compiler is therefore simultaneously:

```text id="kpslzm"
C++ parser
type checker
template engine
reflection implementation
ABI authority
first schema linter
```

`job_schema_gen` should use those facilities rather than rebuilding them.

---

# Compile-Time Reflection Boundary

Schema semantic work that requires C++26 reflection should be completed while reflection metadata is directly available.

The preferred model is:

```text id="s5wofy"
temporary C++ compilation
        │
        ▼
constant evaluation
        │
        ├── inspect schema kind
        ├── inspect members
        ├── inspect annotations
        ├── inspect types
        ├── validate schema policy
        ├── derive generated API
        ├── derive defaults/reset behavior
        ├── derive Packed layout
        └── derive generated source
        │
        ▼
compiled generation result
```

The runtime helper should not need to rediscover what the schema meant.

In particular, it should not reconstruct runtime copies of:

```text id="88ktbl"
member names
C++ type descriptions
annotations
schema hierarchy
layout dependencies
schema policies
```

simply so that another runtime generator can reason about them again.

That work belongs on the compile-time side of the boundary.

---

# Compile-Time Versus Runtime

The architecture intentionally separates two very different jobs.

## Compile-Time Work

While C++26 reflection is available:

```text id="by8xop"
discover the schema declaration
inspect reflected structure
apply job_schema policy
perform compile-time validation
derive generation decisions
compute layout-sensitive information
produce generated source/content
```

This is where the compiler does the meaningful schema work.

---

## Helper Runtime Work

After the temporary helper has been compiled:

```text id="7f8x8k"
open output files
write already-derived generated content
report I/O failures
exit
```

The helper runtime should be deliberately small.

A useful mental model is:

> **The compiler does the thinking. The helper writes down the answer.**

---

# Constant Evaluation

Where practical, generation should use:

```text id="yq6en8"
consteval
constexpr
templates
concepts
static_assert
C++26 reflection
```

to derive output while the C++ declarations remain available directly to the compiler.

This is especially important for operations such as:

```text id="uyt4d3"
member-policy inspection
generated accessor selection
Packed layout
default-value derivation
namespace/type naming
compile-time validation
```

The exact implementation may evolve with compiler support.

The architectural requirement is that reflection-dependent semantic work should not be unnecessarily deferred into a runtime schema-processing engine.

---

# Schema Discovery

The temporary compilation must identify the schema declaration represented by its schema input.

The exact discovery mechanism depends on the available C++26 reflection implementation and the schema-input rules defined elsewhere in the JOB Schema design.

Possible implementation techniques include:

```text id="eodjr1"
reflection-based declaration lookup
compile-time registration
compiler-generated discovery support
another C++26 mechanism
```

The important rule is:

> **Schema identity remains in C++, not duplicated as a second semantic declaration in CMake.**

Schema discovery remains a gating C++26 reflection implementation question and must be proven before the public generator interface is considered stable.

---

# Recursive Compile-Time Generation

Nested generated types may require recursive compile-time reasoning.

The generator should prefer ordinary C++ compile-time recursion and template instantiation over constructing a runtime scheduling system merely to walk type relationships.

Conceptually:

```text id="prk7n3"
generate<Parent>()
    │
    ├── generate<ChildA>()
    │       └── generate<Leaf>()
    │
    └── generate<ChildB>()
```

The compiler already provides many useful properties naturally:

```text id="rra8gb"
type identity
template instantiation
recursive evaluation
compile-time ordering
compile-time diagnostics
deterministic behavior
```

The generator should take advantage of those properties rather than duplicating them in a runtime graph.

---

# Shared Types

When multiple schema relationships refer to the same C++ type, generation should rely on compiler/type identity and reusable compile-time instantiation rather than manually treating each appearance as unrelated.

The important requirement is:

> **The same semantic generated type should not need independently maintained generation state for every place it is referenced.**

Exactly how compiler instantiation and emitter output are deduplicated is an implementation question.

It should remain a C++ generation problem rather than becoming a persisted dependency graph.

---

# Generated `Struct`

`Struct` is the deliberate no-transformation mode.

The input declaration is already the desired ordinary C++ representation.

`job_schema_gen` should therefore perform as little work as practical.

For `Struct`, the correct generated behavior is conceptually:

```text id="ac8o28"
validate schema declaration
preserve ordinary C++ representation
make it available to the generated library
```

It should not add:

```text id="kq833m"
getters
setters
Signals
factories
object inheritance
persistence machinery
copy/move restrictions
```

The generator is doing its job when it recognizes:

> **Nothing needs behavioral generation here.**

The exact build/output handling of Struct source files belongs to the schema-input and build-integration design rather than this document.

---

# Generated `Packed`

`Packed` is the representation for automatic physical-layout generation.

The schema declaration describes the logical fields.

Compile-time generation may:

```text id="67exef"
inspect member types
obtain target size/alignment information
classify members
choose deterministic physical ordering
insert explicit padding
calculate final expected size
emit the physical representation
emit compile-time layout assertions
```

Packed legality is defined by `job_schema`.

`job_schema_gen` does not make an unsupported type legal merely because it can emit some C++ for it.

The generator's job is:

> **Given a valid Packed schema, derive and emit its deterministic physical representation.**

---

# Packed Layout Is Compile-Time Work

Packed layout should be derived while reflected member information and target ABI facts are directly available.

Conceptually:

```cpp id="o80r2n"
template <typename T>
consteval auto layoutOf()
{
    // reflect T
    // inspect its logical members
    // recursively obtain required nested layout facts
    // calculate deterministic ordering/padding
    // return generation-time layout information
}
```

The exact implementation will depend on the compiler's C++26 reflection facilities.

The important architectural rule is:

> **Packed layout is not a runtime scheduling problem.**

There is no requirement for a runtime dependency DAG, thread pool, pipeline, or result-propagation engine in order to compute a Packed type.

---

# Nested Packed Types

When a Packed member contains another generated Packed type by value, the parent layout may depend upon the child's physical representation.

That relationship should be handled recursively during compile-time evaluation.

Conceptually:

```text id="f34dmy"
Parent layout
    │
    └── Child layout
            │
            └── Leaf layout
```

Shared nested types should reuse the compiler's normal type/template identity and instantiation mechanisms where possible.

The generator should not materialize a runtime schema graph solely to model this recursion.

---

# Packed Layout Algorithm

The exact ordering algorithm is implementation policy.

A likely process is:

```text id="ac5o0q"
inspect logical members
determine size/alignment
resolve nested physical facts
order members deterministically
insert explicit padding
compute final expected layout
emit physical C++
```

Generated layout should remain:

```text id="qsnljo"
deterministic
inspectable
ordinary C++
target validated
```

Automatic Packed generation is the:

> **Do the boring layout work for me**

path.

It does not prohibit developers from using ordinary C++ when exact manually controlled physical ordering is the goal.

---

# Packed Final Validation

The generator may calculate an intended Packed representation, but the target compiler remains authoritative.

Generated code should contain useful compile-time checks such as:

```text id="pakjd4"
expected sizeof
expected alignof
standard-layout expectation
trivially-copyable expectation
offset assertions where appropriate
```

The final target compilation therefore validates the emitted physical representation.

This provides two useful compiler gates:

```text id="v0whzh"
schema compilation
    proves the schema and generation assumptions

generated compilation
    proves the emitted C++ representation
```

---

# Generated `BaseObject`

For `BaseObject`, generation may derive ordinary C++ containing:

```text id="l7c8zq"
BaseObject inheritance
member storage
requested member API
JOB pointer aliases
factory helpers
reset support
persistence-compatible state
appropriate lifetime/copy/move policy
```

The generated type must use existing `job_core` persistence facilities.

`job_schema_gen` must not generate a parallel serializer.

---

# Generated `LightObject`

For `LightObject`, generation may derive:

```text id="wov4af"
LightObject inheritance
member storage
requested member API
Signals where required
JOB pointer aliases
factory helpers
runtime lifetime/copy/move policy
```

A generated `LightObject` must remain non-persistent.

---

# Generated `Object`

For `Object`, generation may derive the combined runtime and persistence surface:

```text id="5k1v8n"
Object inheritance
member storage
requested member API
Signals
reset support
persistence-compatible state
JOB pointer aliases
factory helpers
runtime lifetime/copy/move policy
```

The generated result must preserve the existing semantics of `job_core::Object`.

---

# Member API Emission

The generator consumes the member-policy semantics defined by `job_schema`.

The intended model is:

```text id="3wk21q"
no API annotation
    no generated property API

Const
    getter only

RO
    getter + internal mutation path

RW
    getter + public mutation path
```

For signal-capable object kinds:

```text id="ji8rpr"
RO
RW
```

may generate changed Signals.

`Const` has no normal generated mutation path and therefore no ordinary changed Signal.

`Packed` has no generated property API.

---

# Type-Aware APIs

Generated signatures should respect ordinary C++/JOB type conventions.

Generation may distinguish between:

```text id="pu6u3c"
small scalar
enum
large value type
string
container
pointer-like type
```

when deciding whether generated APIs pass or return by value or reference.

The schema author should not have to annotate normal calling-convention details repeatedly.

Those choices should be centralized in generation policy.

---

# Defaults and Reset Generation

C++ member initialization remains the authoritative source of default state.

The generator must not introduce a separately maintained default table.

For example:

```cpp id="u1c6iq"
std::uint32_t timeout{5000};
```

must remain the one declaration of the default.

Depending on the C++26 implementation, generation may derive reset/default behavior using:

```text id="oq04dh"
reflected initializer information
a default-constructed schema input
generated per-member static defaults
another mechanically derived compile-time strategy
```

The implementation is open.

The invariant is not:

> **The C++ schema initializer is the single source of truth for the generated default.**

---

# Signal Generation

Signals are generated behavior rather than schema data.

For example:

```cpp id="44mghk"
[[=job::schema::RW{}]]
std::string name{};
```

inside a signal-capable generated type may produce:

```text id="0eebsi"
getter
setter
nameChanged Signal
```

The schema author should not need to separately repeat that Signal declaration.

Signal signatures should follow the same type-aware conventions as their corresponding generated member API.

---

# Contract Generation

`job_schema_gen` may emit contracts only when the invariant is mechanically known from generated behavior.

It must not invent application semantics.

Potentially appropriate generated contracts include:

```text id="vptfee"
postconditions around generated state changes
internal generated assumptions
layout-related invariants
```

It must not guess rules such as:

```text id="1jzr8c"
width > 0
name is not empty
count < 64
```

unless some schema policy explicitly expresses those semantics.

Contracts protect programmer/runtime invariants.

They do not replace input or domain validation.

---

# Generation Support Ownership

Some generation code must exist inside the temporary compilation because that is where reflected declarations are directly available.

That code may conceptually come from:

```text id="6e3mhf"
job_schema
    schema semantics
    reflection helpers
    compile-time validation

job_schema_gen
    code emission
    generation templates
    generated-source construction
```

Some `job_schema_gen` support may therefore need to be header-visible or otherwise compiled into the temporary helper.

That does not move generation ownership into `job_schema`.

Compilation location and architectural ownership are separate concerns.

---

# Generation Helper

The generation helper is a temporary compiler-produced artifact that materializes the already-derived generated source into files.

It is not:

```text id="h6byrw"
installed
exported
linked by consumers
a public CMake target
a generated-library runtime dependency
```

Its lifetime exists only as part of the build.

---

# Helper Runtime

The helper runtime should remain small.

Conceptually:

```text id="juqa7z"
main()
    ↓
write generated header
write generated source
    ↓
return success/failure
```

It should not need to:

```text id="fr1hcd"
re-reflect schemas
rebuild schema policy
perform runtime layout analysis
construct a runtime schema graph
rediscover annotations
reimplement the compiler's type system
```

This keeps the boundary between compilation and runtime clear.

---

# Helper Form

A temporary executable is a strong initial implementation candidate.

It provides one simple model for both native and cross builds:

```text id="4d23gj"
native
    compile helper
    run helper directly

cross
    compile target helper
    run helper through configured emulator
```

The design does not require a temporary shared-object architecture.

Another helper form may be used if implementation experiments demonstrate a clear advantage.

The simplest reliable mechanism should win.

---

# Native Execution

When the generated helper can execute directly on the build host:

```text id="8h99gg"
target compiler
    ↓
generation helper
    ↓
execute directly
    ↓
generated files
```

---

# Cross Execution

When the generation helper is target code that cannot execute directly on the host:

```text id="ebmtnb"
target compiler
    ↓
target generation helper
    ↓
configured emulator / QEMU
    ↓
generated files
```

QEMU or another emulator is only the execution mechanism.

It does not determine:

```text id="6445eh"
target ABI
sizeof
alignof
schema validity
reflection results
```

Those facts were already determined by the target compiler.

---

# Process Execution

`job_schema_gen` should use JOB's process/I/O facilities rather than embedding its own private process implementation.

The generator needs a process abstraction capable of performing work such as:

```text id="q037zl"
run compiler
run linker
run generation helper
run configured emulator
capture diagnostics
obtain exit status
```

Whether the lower-level JOB process implementation uses pipes, a PTY, or another execution mode is not part of the Schema generator's semantic design.

`job_schema_gen` should consume that capability rather than implementing `fork()`/`exec()` machinery itself.

---

# Toolchain Input

The generator needs enough information to compile the temporary schema helper in the same relevant target environment as the final generated code.

Inputs may include:

```text id="v3a2wn"
C++ compiler
C++ standard
compiler options
include paths
compile definitions
sysroot
target architecture options
reflection options
contract options
linker options
configured emulator
schema source
output paths
```

The exact command-line representation remains an implementation detail.

Possible transport mechanisms include:

```text id="1cysc1"
structured command-line options
response files
CMake-generated configuration input
```

The chosen mechanism should handle large real-world build configurations without fragile shell quoting.

---

# Temporary Compilation

`job_schema_gen` may construct a temporary C++ translation unit that includes the schema input and required generation support.

Conceptually:

```cpp id="ydgtqq"
#include "device_schema.h"

#include <job_schema/...>
#include <job_schema_gen/...>

// generation entry point
```

This translation unit is generated build machinery.

The developer should not maintain it manually.

Temporary compiler artifacts may include:

```text id="9njqf3"
temporary source
response file
object files
helper executable
dependency files
logs
staging output
```

None of these are public library products.

---

# Temporary Build Workspace

Temporary artifacts should live in a build-owned, invocation-specific location.

Conceptually:

```text id="hpk8um"
<binary-dir>/.job_schema/<generated-target-or-unit>/
```

The exact layout belongs to build integration.

The important requirements are:

```text id="tr5wr2"
parallel-build safety
debuggability
no global shared temporary state
clear ownership by the consuming build
```

---

# Debug Preservation

Normally temporary compiler/helper artifacts may be cleaned when no longer needed.

During generator development, preserving them should be possible.

Useful debug artifacts include:

```text id="ubdylz"
temporary TU
compiler arguments/response file
link invocation
helper binary
helper diagnostics
generated staging files
```

The exact user-facing debug option is an implementation/API decision.

---

# Generated Output

The generator emits ordinary C++.

Likely artifacts include:

```text id="wlhxnd"
public headers
implementation sources
required includes
export decoration
static assertions
generated contracts where appropriate
```

The exact header/source split is implementation policy.

Generated source should be:

```text id="a6pq3f"
readable
deterministic
inspectable
ordinary C++
compatible with normal C++ tooling
```

There should be no hidden runtime generator or interpreter dependency.

---

# Generated Code Quality

Generated C++ is still JOB C++.

Where applicable it should follow JOB conventions for:

```text id="61ti5g"
namespaces
naming
formatting
pointer aliases
factory helpers
lifetime policy
Signal conventions
contracts
include style
visibility/export handling
```

The generator should not produce stylistically alien C++ merely because a machine wrote it.

Readable generated output is valuable during debugging.

---

# Namespace Preservation

Generated types must preserve their intended C++ namespace identity.

A schema declaration inside:

```cpp id="2udqd2"
namespace job::foo {
    ...
}
```

must not silently become a global type or a type in some public generator-owned namespace.

Namespace identity is already present in C++ and should be derived through reflection.

---

# Type Naming

The mapping between schema declaration identity and final generated type name must be deterministic.

The temporary schema input and final generated declaration may require distinct internal identities during generation.

The implementation must solve that without forcing awkward permanent user-facing names solely for the generator.

The exact technique remains an implementation question tied closely to C++26 reflection capabilities.

---

# Include Generation

Generated source should contain the includes it actually requires.

Those may include:

```text id="a2oiam"
JOB object headers
Signal
memory
strings
containers
user-defined referenced types
generated export headers
other generated declarations
```

C++ relationships should remain authoritative.

The generator should not introduce another dependency-description language merely to decide includes.

---

# Export Visibility

Generated libraries may be static or shared.

When shared-library generation requires visibility decoration, `job_schema_gen` must be able to emit the appropriate JOB export macro and include.

The build layer may provide:

```text id="965yqa"
export-header include
export macro name
```

The exact CMake-facing contract belongs in the build-integration design document.

---

# Deterministic Generation

Equivalent semantic inputs should produce equivalent generated source.

The generator should avoid unnecessary dependence on:

```text id="2oyakt"
current time
random IDs
temporary absolute paths
unrelated environment state
nondeterministic ordering
```

Relevant compiler and target configuration are legitimate generation inputs.

Generated ordering should be explicitly deterministic.

Because meaningful schema generation occurs during constant evaluation rather than through a runtime worker scheduler, concurrency ordering should not affect emitted semantics.

---

# Avoiding Unnecessary Rewrites

Where practical, generated output files should only be replaced when their contents actually change.

This helps avoid unnecessary:

```text id="g10wxm"
timestamp changes
Ninja rebuilds
downstream recompilation
relinking
CI noise
```

The exact compare/write/rename mechanism belongs to normal `job_io` and build implementation work.

---

# Incremental Builds

CMake/Ninja remain responsible for deciding when generation reruns.

Relevant inputs may include:

```text id="5lsocu"
schema source
transitive includes
job_schema
job_schema_gen
generation configuration
compiler configuration
toolchain configuration
```

Compiler-generated dependency information may be used where useful.

`job_schema_gen` should not maintain its own competing build database.

---

# Diagnostics

Compiler and linker diagnostics should be forwarded as faithfully as practical.

Useful information includes:

```text id="jl95ik"
stdout
stderr
exit status
source path
line/column
compiler command in verbose/debug mode
```

A compiler error should remain recognizable as a compiler error.

`job_schema_gen` should not turn:

```text id="lrzofc"
device_schema.h:42:17: ...
```

into only:

```text id="kxkn09"
generation failed
```

---

# Logging

`job_schema_gen` should use normal JOB logging/error facilities.

It should not create a generator-specific logging framework.

Useful categories may include:

```text id="dwjroc"
generation status
verbose compiler invocation
temporary-artifact information
warnings
errors
```

The exact APIs depend on the JOB logging design in use when the application is implemented.

---

# Failure Model

Generation has a small number of clear failure stages.

## Schema Compilation Failure

Examples:

```text id="2m7dr8"
invalid C++
missing include
failed concept
invalid schema annotation
invalid Packed type
reflection compilation failure
target compiler error
```

Result:

```text id="b71drs"
surface compiler diagnostics
stop generation
```

---

## Link Failure

If the helper requires linking and that step fails:

```text id="944fk9"
surface linker diagnostics
stop generation
```

---

## Helper Execution Failure

Examples:

```text id="ug87gy"
helper exits nonzero
helper crashes
configured emulator fails
runtime contract violation
```

Result:

```text id="x30cag"
surface helper/emulator diagnostics
stop generation
```

---

## File Output Failure

Examples:

```text id="oixhlj"
output directory unavailable
permission error
disk full
write failure
rename failure
```

Result:

```text id="gdp364"
report through normal JOB I/O/error facilities
stop generation
```

The error should identify which generation stage failed.

---

# Reproducibility

A generation invocation should be reproducible from its meaningful inputs.

The generator should avoid hidden reliance on:

```text id="lsz1sz"
current working directory where unnecessary
system time
random state
unrelated environment variables
global temporary names
```

The target compiler/toolchain environment is a legitimate input and should be supplied explicitly by the build where practical.

---

# Security and Trust Model

Schema input is project C++ source.

The temporary helper therefore executes trusted build code in the same sense as other project build tooling.

`job_schema_gen` is not intended to sandbox arbitrary hostile C++.

Process and filesystem handling should nevertheless follow normal safe JOB conventions:

```text id="11kywh"
structured process arguments
avoid unnecessary shell interpolation
controlled temporary paths
validated output paths
parallel-safe build state
```

---

# Product Requirements

The implementation of `apps/job_schema_gen` should satisfy the following requirements.

1. Schema input is real C++26.

2. `job_schema_gen` must use the actual configured target compiler as part of generation.

3. C++26 reflection is the source of structural knowledge about schema declarations.

4. The architecture should depend on sufficiently complete C++26 reflection capabilities rather than permanently defining GCC-specific behavior as part of the schema format.

5. The compiler is the first schema linter.

6. Compiler diagnostics must remain visible and useful.

7. `job_schema_gen` must not contain an independent C++ parser.

8. `job_schema_gen` must not maintain a duplicate schema-policy engine.

9. It must not require a canonical serialized schema IR.

10. Reflection-dependent semantic generation should occur while reflection information is directly available during compilation/constant evaluation.

11. The runtime helper should not reconstruct a runtime schema model.

12. The runtime helper should primarily materialize already-derived generated content into files.

13. Schema discovery must be proven against the selected C++26 reflection implementation before the public generator interface is considered stable.

14. Recursive generated-type relationships should use normal compile-time C++ mechanisms where practical rather than a runtime generation scheduler.

15. `Struct` remains the minimal/no-behavioral-transformation mode.

16. `Packed` layout is derived during compile-time generation.

17. Packed legality remains the responsibility of schema validation.

18. Packed generation may deterministically reorder fields and add explicit padding.

19. Packed output must remain readable and inspectable.

20. Packed physical assumptions must be validated by the target compiler.

21. Generated BaseObject/Object types must reuse existing `job_core` persistence.

22. Generated LightObject types must remain non-persistent.

23. Generated member APIs must follow schema policy rather than generator-local reinterpretation.

24. Defaults must derive from C++ member initialization.

25. Signals must derive from schema kind and member policy.

26. Generated contracts must be limited to mechanically known invariants.

27. Generated output must be ordinary C++ with no generator runtime dependency.

28. Generated output should follow JOB coding conventions where applicable.

29. Namespace identity must be preserved.

30. Type naming must be deterministic.

31. Native helper execution must be supported.

32. Cross-target helper execution must be supported through a configured emulator when required.

33. QEMU or another emulator must not be treated as the source of target ABI/reflection facts.

34. Compiler, linker, helper, and emulator execution should use JOB process/I/O facilities rather than private process implementations inside Schema.

35. Temporary build artifacts must remain implementation details rather than public build products.

36. Generated output should be deterministic for equivalent semantic inputs.

37. Generated files should avoid unnecessary rewrites where practical.

38. Failures should clearly identify the stage in which they occurred.

39. `job_schema_gen` must not manage public CMake targets or installation policy.

---

# Decisions Already Made

The following are current architectural decisions:

```text id="a7ffz4"
job_schema_gen is a compiler-aware C++ source generator.

The schema input is real C++26.

The target compiler is part of generation.

C++26 reflection is the source of structural knowledge.

job_schema owns schema semantics.

job_schema_gen owns generation/emission mechanics.

CMake owns build orchestration.

There is no canonical serialized schema IR.

Reflection-dependent generation belongs on the compile-time side.

The compiler performs the meaningful schema analysis.

The helper runtime primarily writes already-derived output.

Runtime generation does not require a schema DAG.

Runtime generation does not require JobThreadGraph or JobPipeline.

Recursive type relationships should use normal compile-time C++ mechanisms where practical.

Struct remains the minimal/no-transform mode.

Packed layout is a compile-time generation problem.

Packed legality is decided before emission.

The target compiler validates final Packed representation.

BaseObject/Object reuse job_core persistence.

LightObject remains non-persistent.

Member API generation follows job_schema policy.

Native helpers execute directly.

Cross-target helpers execute through the configured emulator when required.

The emulator does not supply target ABI or reflection facts.

Process execution is delegated to JOB process/I/O facilities.

Temporary helper artifacts are build implementation details.

Generated source is ordinary, deterministic, inspectable C++.

Schema discovery gates stability of the generator interface.
```

---

# Gating Implementation Work

Only questions that materially determine the generator architecture belong here.

## 1. Schema Discovery

Prove how the temporary compilation identifies the schema declaration using the available C++26 reflection implementation.

This affects:

```text id="6i93q7"
temporary TU construction
generation entry point
CMake invocation
type naming
```

This must be resolved before the public generation interface is frozen.

---

## 2. Compile-Time Source Generation

Prove the cleanest mechanism for turning reflection results into generated C++ content during constant evaluation.

Questions include:

```text id="eo106e"
how source text is accumulated
what constexpr containers are practical
how large generated outputs behave
what compile-time helpers belong to job_schema_gen
how the derived output is exposed to helper runtime
```

This is the central generator experiment.

---

## 3. Default Materialization

Prove one mechanism that derives reset/default behavior solely from the C++ schema member initializer.

Candidate approaches include:

```text id="96v30b"
reflection of initializer information
default-constructed schema input
generated per-member static defaults
```

No second hand-maintained default declaration may be introduced.

---

## 4. Packed Layout

Build a nested Packed prototype proving that compile-time recursive layout can derive:

```text id="7lx9a5"
nested physical size
alignment
deterministic ordering
padding
final expected size
target compile assertions
```

without requiring a runtime schema graph.

---

## 5. Helper Materialization

Prove the simplest reliable path from compile-time-generated content to files.

A temporary executable is the leading simple model.

The experiment should work for both:

```text id="6c2i13"
native execution
cross-target execution through configured emulator
```

---

## 6. Toolchain Propagation

Prove how the build supplies the effective target compiler configuration to `job_schema_gen`.

The mechanism must handle real-world:

```text id="z7gpjg"
include paths
compile definitions
sysroots
reflection flags
contracts flags
target flags
cross compilation
```

without fragile command construction.

---

# Open Design Questions

## Compile-Time Text Representation

How should generated text be accumulated during constant evaluation?

Possible approaches include:

```text id="9hh2pt"
compile-time string builder
fixed/derived arrays
compiler-supported constexpr containers
fragment-based generation
```

The implementation should remain reasonably simple and scale to realistic generated objects.

---

## Header / Source Split

How much generated implementation belongs in:

```text id="osqzs5"
generated header
generated source
```

?

The answer may differ by schema kind.

It should favor normal C++ library practices rather than arbitrary generator symmetry.

---

## Type Naming During Generation

How do temporary schema declaration identities and final generated public type names coexist during compilation?

The solution should not require ugly permanent names in user schema code merely to satisfy the generator.

---

## Default Representation

Which compiler-supported mechanism gives the cleanest mechanically derived reset/default implementation?

This should be answered through a small C++26 reflection prototype rather than speculation.

---

## Packed Ordering

What deterministic automatic packing strategy should be used?

Possible goals include:

```text id="17sj0t"
minimum wasted space
stable alignment grouping
preservation of selected ordering constraints
simple understandable generated output
```

The first implementation should favor correctness and determinism over an unnecessarily clever optimizer.

---

## Source Formatting

Should source generation:

```text id="w9vtak"
emit already-formatted C++ directly
use a small internal formatting writer
run a separate formatter
```

?

An external formatter should not become a required dependency without a concrete reason.

---

## Version Metadata

Does generated source need build-time metadata identifying:

```text id="25y1cg"
job_schema_gen version
generation format version
required schema ABI/version
```

?

Only metadata with a concrete compatibility or diagnostic purpose should be added.

---

# Design Principles

The implementation should repeatedly return to a few rules.

> **Do not parse what the compiler already understands.**

> **Do not rebuild schema semantics in the generator.**

> **Do not serialize reflection merely to deserialize it into another schema model.**

> **Do reflection-dependent work while reflection is directly available.**

> **Do not turn compile-time recursion into a runtime scheduler without evidence that one is needed.**

> **Do not make generated code depend on the generator at runtime.**

> **Do not make CMake understand C++ schema semantics.**

> **Do not hide useful compiler diagnostics behind generator abstractions.**

> **Do not make `job_schema_gen` design lower-level JOB libraries merely because it consumes them.**

The central compile-time question is:

> **What can the compiler derive directly from this schema declaration?**

The central generator question is:

> **How little work remains after the compiler has derived it?**

The desired answer is:

```text id="8iv7ay"
small C++ schema
      ↓
target compiler
      ↓
C++26 reflection + constant evaluation
      ↓
already-derived generated C++
      ↓
small helper runtime
      ↓
write files
      ↓
normal JOB library
```

The compiler does the thinking.

The helper writes the answer.

That is the job.
