# job_schema Design

> **Document purpose:** This is a design and implementation-planning document for `libs/job_schema`.
>
> It records the requirements, architectural boundaries, rationale, expected behavior, integration work, and open design questions for the library. It is not API reference documentation and should not be treated as a promise of exact class names, header names, or implementation details.
>
> For the complete JOB Schema pipeline, see [JOB Schema — High-Level Architecture](intro.md).
>
> `job_schema` builds on the object, reflection, serialization, and contract facilities provided by [JOB Core](../../job/core_overview.md).

---

# Purpose

`job_schema` is the compile-time backing library for JOB Schema.

Its purpose is to provide the vocabulary and rules needed to turn small C++26 schema declarations into one of five supported generated representations:

```text
Struct
Packed
BaseObject
LightObject
Object
```

The library exists so that a developer describes a field once and lets reflection and generation derive the repetitive implementation around that field.

For clarity, this design document generally shows the underlying annotation form directly:

```cpp
[[=job::schema::RW{}, =job::schema::Required{}]]
std::string name{};
```

From that declaration, the schema system already has, or can derive:

```text
member name
member type
default initialization
annotations
containing schema kind
member ordering
array/container information
size/alignment where relevant
```

The developer should not have to repeat those facts elsewhere.

The generated representation can then derive the appropriate:

```text
backing member
getter
setter
reset behavior
changed Signal
persistence behavior
presence checks
copy/move policy
pointer aliases
factory helpers
physical layout
```

depending on the selected schema kind and member policies.

The primary design goal is therefore:

> **The C++ schema declaration is the single source of truth for generated structure and behavior.**

---

# Design Goals

`job_schema` should satisfy the following goals.

## Use Real C++

Schema declarations are real C++26.

The schema system should rely on the compiler for:

```text
syntax
names
types
namespaces
templates
enums
includes
inheritance
initialization
type relationships
compile-time diagnostics
```

JOB should not create a second type system merely to describe C++.

This is a major reason for moving away from YAML/JSON as the canonical schema language.

---

## Use Reflection Instead of Repetition

If the compiler can already answer a question through reflection, the schema should not require the developer to write the answer again.

The schema should not need redundant declarations such as:

```text
member name
member count
sizeof
alignment
C++ type spelling
base-class identity
array extent
```

where those facts are already represented by the C++ type.

Annotations should exist only where they express **intent or policy that C++ alone does not communicate**.

---

## Fail at Compile Time

Invalid schema declarations should fail during the schema compilation stage.

The compiler, concepts, static assertions, contracts where appropriate, and C++26 reflection form the first schema linter.

A bad schema should produce a normal compilation failure rather than surviving until a later generator pass where practical.

Examples include:

```text
invalid annotation combinations
unsupported member types
invalid Packed members
illegal policy for a schema kind
failed schema concepts
invalid target-layout assumptions
```

The intended developer experience is:

```text
bad schema
    ↓
ordinary compiler diagnostic
    ↓
fix declaration
```

rather than:

```text
bad schema
    ↓
generator-specific parser
    ↓
secondary error language
```

---

## Preserve JOB Runtime Semantics

Generated object families must behave like normal JOB objects.

`job_schema` must build on the existing semantics of:

```text
BaseObject
LightObject
Object
Signal
Connection
NoSerialize
NoReset
reflection serialization
reset
contracts
```

It must not create parallel versions of those systems.

---

## Keep Generation Policy Separate From Generation Mechanics

`job_schema` defines:

> What is valid?

and:

> What does a schema declaration mean?

It does not own:

```text
compiler process execution
temporary directories
linker invocation
QEMU execution
CMake target creation
file emission
generated source installation
```

Those belong to `job_schema_gen`, `job_io`, and `job_schema_gen.cmake`.

---

# Annotation Types and Shorthand

The schema annotations themselves should have one canonical semantic definition.

A likely split is:

```text
job_schema_annotations.h
    annotation types and their meaning

job_schema_alias.h
    convenient public shorthand values/forms
```

For example, the underlying annotation may be:

```cpp
[[=job::schema::RW{}]]
std::string name{};
```

while `job_schema_alias.h` may eventually provide a shorter stable spelling conceptually similar to:

```cpp
namespace job::schema {

inline constexpr RW JOB_RW{};
inline constexpr RO JOB_RO{};
inline constexpr Const JOB_CONST{};
inline constexpr Required JOB_REQUIRED{};

} // namespace job::schema
```

allowing:

```cpp
[[=job::schema::JOB_RW, =job::schema::JOB_REQUIRED]]
std::string name{};
```

The exact syntax is subject to the C++26 annotation/reflection implementation available when this is built.

This design document intentionally uses the longer:

```cpp
[[=job::schema::RW{}]]
```

form because it makes the underlying policy explicit.

The shorthand layer exists for developer ergonomics and for some insulation from implementation changes.

Before JOB Schema reaches its first stable release, the project reserves the right to change these convenience spellings as the annotation implementation evolves.

After the stable `0.1` interface is declared, public shorthand should not be changed casually.

The shorthand layer may eventually grow beyond member-policy values if useful.

For example, future convenience forms could potentially wrap common schema declarations:

```text
JOB_OBJECT(...)
JOB_LIGHT_OBJECT(...)
JOB_PACKED(...)
```

or other repetitive schema syntax.

Those are not requirements today.

The important rule is:

> **Shorthand may change spelling, but it must resolve back to the same underlying C++ schema annotations and semantics.**

The generator must reason about the annotation types and reflected semantics, not about a particular convenience spelling.

---

# Non-Goals

The following are explicitly outside the role of `job_schema`.

## It Is Not Another Serializer

`job_core` already provides reflection-based persistence.

`job_schema` should not create another serialization framework merely because it is inspecting reflected members.

Generated `BaseObject` and `Object` types must use the existing `job_core` persistence machinery.

If the goal is explicit MessagePack-based serialization rather than schema-driven code generation, see the [legacy JOB Serializer](../../job/serializer_overview.md).

The existing serializer remains the appropriate place for code that specifically requires its serializer model or MessagePack representation.

JOB Schema does **not** attempt to reproduce the MessagePack wire format used by `job_serializer`, and wire compatibility with that format is not a design goal.

The long-term relationship between the legacy serializer stack and reflection-driven JOB persistence can be decided separately after JOB Schema is implemented and tested.

---

## It Is Not an Intermediate Schema Language

There should not be a required architecture such as:

```text
C++ schema
    ↓
SchemaDescription
    ↓
SchemaMemberDescription
    ↓
serialize IR
    ↓
generator
```

The reflected C++ declarations are already the schema.

A temporary internal data structure may eventually be useful as an implementation convenience, but it must not become:

```text
a second authoritative schema
a public compatibility format
a serialization boundary
a required generator interface
```

In particular, the design should avoid accidentally rebuilding:

```text
YAML schema
JSON schema
MessagePack schema
custom schema IR
```

behind the C++ front end.

The generator should operate from reflected C++ semantics as directly as the implementation permits.

---

## It Is Not the Build System

`job_schema` does not know:

```text
CMAKE_CXX_COMPILER
CMAKE_SYSROOT
CMAKE_CROSSCOMPILING
QEMU
temporary build paths
target names
install destinations
```

Those are build-system concerns.

---

## It Is Not Domain Validation

Schema validation and application/domain validation are different things.

`job_schema` may enforce:

```text
schema legality
member-policy legality
presence requirements
generation requirements
```

It does not replace application logic that may return human-readable domain errors such as:

```text
manifest.models[2].path is empty
manifest.dependencies[4].version is invalid
device combination is unsupported
```

Those remain application responsibilities.

Bad user input is not automatically a programmer error and should not be converted into contract violations.

---

# Relationship to `job_core`

`job_schema` depends heavily on `job_core`.

The relationship is:

```text
job_core
    runtime object model
    reflection persistence
    annotations
    contracts
    signals/connections

        ↑

job_schema
    schema vocabulary
    schema policy
    schema reflection helpers
    schema concepts
    compile-time schema validation
```

The direction matters.

`job_core` must remain usable without `job_schema`.

`job_schema` is a higher-level compile-time helper built on top of core facilities.

---

# Required `job_core` Integration

`Required` is the first schema-owned policy that requires explicit integration work inside `job_core`.

This integration should be treated as one of the first implementation tasks once the annotation itself exists.

`job_schema` defines:

```text
Required
```

but `job_core` owns the persistence readers that still know whether a key was actually present.

Therefore `job_core` readers must eventually be updated to:

```text
recognize reflected Required policy
detect absent required fields
accumulate missing-field diagnostics
report them through normal JOB error/logging facilities
```

The important ownership distinction is:

```text
job_schema
    defines policy

job_core
    enforces the policy while performing an operation it owns
```

This should not create a schema-specific parser or error system.

The existing JOB logging/error direction should be reused, especially as `job_logger` and contract reporting continue to converge.

---

# Schema Source Model

The schema source is normal C++.

A schema input should require as little schema-specific syntax as possible.

Conceptually:

```cpp
[[=job::schema::Object{}]]
struct Device {
    [[=job::schema::RW{}, =job::schema::Required{}]]
    std::string name{};

    [[=job::schema::RO{}]]
    std::uint32_t state{};

    [[=job::schema::Const{}]]
    std::uint32_t capabilities{};
};
```

The exact syntax remains subject to the C++26 reflection/annotation implementation available in the compiler.

The design intent is:

```text
the struct describes data
the type annotation selects output kind
member annotations describe policy
reflection supplies everything else
```

The input declaration is intentionally much smaller than the final generated representation.

---

# The Five Schema Kinds

The schema kind controls the broad generated representation.

These five kinds are generator modes, not one inheritance hierarchy.

```text
Struct
Packed
BaseObject
LightObject
Object
```

---

# `Struct`

`Struct` means:

> The schema declaration already represents the desired plain C++ structure.

No object framework should be added.

It should not receive generated:

```text
pointer aliases
factory methods
signals
getters/setters
persistence integration
copy/move restrictions
```

The primary purpose of `Struct` is to let a schema set contain normal C++ structures alongside generated object types.

In many cases the schema header may simply become part of the generated library's public interface without transformation.

`Struct` exists specifically because sometimes the right generated representation is:

> **Nothing. The C++ the developer wrote is already correct.**

The generator should do its job by recognizing that and leaving it alone.

---

# `Packed`

`Packed` means:

> Generate a fixed-layout, compact, memcpy-friendly physical representation from the logical schema declaration.

Unlike `Struct`, the generator is explicitly allowed to transform physical ordering and introduce explicit padding.

This exists so the developer does not have to manually solve layout problems every time.

The generator may:

```text
reorder members
group members by alignment
insert explicit padding
round structures to an intentional size
emit layout assertions
```

The resulting type is intended for uses such as:

```text
memcpy
mmap
shared memory
fixed binary records
compact transport records
```

A Packed type should remain deliberately simple.

It should not gain:

```text
virtual functions
signals
factory helpers
pointer aliases
property APIs
runtime ownership semantics
```

The physical representation must remain obvious and cheap.

---

# Packed API Policy

Generated API annotations such as:

```text
RW
RO
Const
```

are invalid for `Packed`.

This is intentional, not a missing feature.

A Packed representation has no generated property/accessor API.

Its members are the physical record.

If the developer needs generated accessors, signals, mutation policy, or behavioral object semantics, the schema should use one of the object families rather than `Packed`.

---

# Packed Design Requirements

`Packed` deserves special rules because its purpose is different from the object families.

A valid Packed schema should normally require member types whose physical representation can be reasoned about safely.

Likely requirements include concepts around:

```text
trivially copyable representation
fixed-size members
standard-layout generated result
known-size arrays
fixed-width integer types
enums with known underlying representation
nested Packed-compatible structures
```

Likely rejected or specially handled types include:

```text
std::string
std::vector
shared_ptr
unique_ptr
weak_ptr
raw ownership pointers
virtual bases
runtime-polymorphic members
```

The exact permitted type set should be decided during implementation and tested rather than guessed in advance.

The target compiler is the final authority for:

```text
sizeof
alignof
member layout
target ABI
```

Generated Packed output should include compile-time assertions where useful so incorrect assumptions fail during final target compilation.

---

# Packed Layout Policy

The generator, rather than the schema author, may perform common layout optimization work.

For example, a logical declaration:

```cpp
struct Example {
    std::uint8_t mode{};
    std::uint64_t offset{};
    std::uint32_t count{};
};
```

may generate a physical representation ordered differently to reduce wasted space.

Where padding remains necessary, it should be explicit in the generated type.

The final C++ layout should be inspectable.

The generator should not hide physical padding in an opaque binary representation.

The result should still be understandable as ordinary C++.

A developer who wants to manually control physical layout should remain able to express that directly rather than being forced through the automatic Packed optimizer.

`Packed` exists partly as the "do this boring layout work for me" mode.

---

# `BaseObject`

`BaseObject` means:

> Generate a persistence-oriented JOB object derived from the existing `BaseObject`.

The generated representation should receive the JOB conventions appropriate to this family.

Likely generated behavior includes:

```text
BaseObject inheritance
Ptr / WPtr / UPtr aliases
createShared()
createUniq()
generated member API where requested
reflection persistence integration
reset integration
```

The exact special-member behavior should follow the actual semantics of `BaseObject`.

The generator should not mechanically emit boilerplate merely because a generic "Rule of Five" checklist exists.

The correct copy/move/lifetime behavior should be derived from the intended object semantics and verified against the existing `BaseObject` design.

---

# `LightObject`

`LightObject` means:

> Generate a runtime JOB object with identity, signals, and connection behavior, but without `BaseObject` persistence.

The generated representation should derive from the existing `LightObject`.

Likely generated behavior includes:

```text
LightObject inheritance
Ptr / WPtr / UPtr aliases
createShared()
createUniq()
generated property API
Signal generation
connection behavior
JOB copy/move/lifetime policy
```

`LightObject` remains its own hierarchy.

It does not derive from `BaseObject`.

This distinction must remain visible throughout the schema design because persistence is the primary semantic difference between `LightObject` and `Object`.

---

# `Object`

`Object` means:

> Generate the full JOB runtime object with both persistence and signal/connection behavior.

The generated representation derives from the existing `Object`.

`Object` combines:

```text
BaseObject persistence
runtime identity
Signal management
connection management
generated member APIs
pointer aliases/factories
JOB lifetime policy
```

The runtime hierarchy is:

```text
BaseObject
    ▲
    │
  Object


LightObject
    separate hierarchy
```

The schema kind selection must preserve this existing distinction rather than creating a parallel inheritance model.

---

# Object Abstractness

`Object` and `LightObject` should not require a meaningless pure virtual `isValid()` function merely to remain abstract.

`isValid()` mixes together several unrelated questions and does not belong in the runtime object abstraction.

If the base types should remain non-instantiable, abstractness should be expressed structurally rather than by inventing a behavioral requirement that derived classes do not universally need.

The exact mechanism remains an implementation decision.

Possible approaches should be evaluated against:

```text
JOB factory conventions
constructor visibility
destructor behavior
concept requirements
derived-class usability
```

The design requirement is:

> **`Object` and `LightObject` may remain architectural base types without forcing every derived type to implement a universal validity query.**

---

# Member Policy Model

Schema members may carry policies that affect generated behavior.

The initial policies divide naturally into separate categories.

---

# Generated API Policies

These policies are owned by `job_schema`.

```text
RW
RO
Const
```

Their purpose is to control generated API around a member.

They should not redefine the member's type or duplicate reflection facts.

An unannotated member is deliberately distinct from all three.

The intended progression is:

```text
no annotation
    no generated accessor API

Const
    read-only generated API

RO
    read API + internal mutation API

RW
    read API + public mutation API
```

This distinction prevents `Const` and an unannotated member from collapsing into the same policy.

---

## No API Annotation

A member with no API-policy annotation means:

> **Do not generate a property API for this member.**

For generated object families, the member may still participate in:

```text
reflection
persistence
reset
internal generated structure
```

where appropriate to its containing schema kind.

But it does not automatically receive:

```text
getter
setter
changed Signal
```

`RW`, `RO`, and `Const` exist specifically to request those API surfaces.

No annotation should mean as little generated policy as practical.

It must not silently mean `RW`.

---

## `RW`

`RW` means the generated public API supports both reading and writing.

Depending on containing schema kind, this may generate:

```text
getter
public setter
reset operation
changed Signal
```

The exact signatures should be derived from member type.

For example, large value types may use references while small scalar types may reasonably use values.

Those calling conventions are generator policy, not schema syntax.

---

## `RO`

`RO` means:

> Externally read-only through the generated API.

It does not mean the underlying state can never change.

A generated `RO` member may still receive an internal or protected setter so the implementation can update it.

For signal-capable object kinds, internal changes may still emit a changed Signal.

This follows the broad semantic distinction already familiar from JOB's historical Qt property helpers:

```text
public read
internal write
```

---

## `Const`

`Const` is a stronger generated API restriction than `RO`.

Its intended meaning is:

```text
getter
no public setter
no generated internal setter
no changed Signal
```

Reset behavior remains a separate decision because persistence reconstruction and reset semantics are not identical to public API mutability.

Most importantly:

> **`Const` controls the generated API. It does not make persisted storage immutable.**

Reflection-based reconstruction may still populate a Const-policy member when loading:

```text
JSON
YAML
binary
```

If true immutable storage is ever required, that must be treated as a separate design problem rather than overloading `Const`.

---

# Input Presence Policy

`Required` is also owned by `job_schema`, but its enforcement crosses into `job_core`.

```text
Required
```

means:

> **The member must be explicitly present when reading an input representation that can distinguish a missing field from a present field.**

This definition is intentionally independent of member type.

For example:

```json
{}
```

fails the presence rule for a required integer field.

But:

```json
{
    "count": 0
}
```

satisfies the presence rule.

`Required` must **not** mean:

```text
nonzero
nonempty
different from default initializer
truthy
```

Those are value semantics and differ by type and application.

Presence is the only meaning that remains consistent across all member types.

---

# `Required` and `job_core`

Presence information exists during parsing.

After the object has been constructed, the difference between:

```text
field absent
```

and:

```text
field explicitly present with its default value
```

may no longer exist.

Therefore enforcement belongs in the persistence reader.

This creates an intentional ownership split:

```text
job_schema
    defines Required

job_core
    enforces Required while parsing
```

For example, a persistence reader currently shaped like:

```cpp
if (j.contains(key)) {
    // deserialize member
}
```

will eventually need to account for:

```text
member is missing
+
member carries Required
```

and report that condition.

This is not generated `isValid()` logic.

It belongs exactly where missing-key information still exists.

---

# Required Error Reporting

Missing required fields should normally be accumulated during one parsing operation.

A user should not need to fix one missing key, retry, discover the next, and repeat.

For example:

```text
Missing required fields:
  width
  height
  format
  device
```

is preferable to stopping at `width`.

The exact error container and formatting belong to `job_core`, `job_logger`, and the broader JOB error-reporting design rather than to `job_schema`.

`job_schema` only needs to provide enough reflected policy information for the persistence layer to identify missing required members.

A schema-specific diagnostic framework should not be created for this.

---

# Existing `job_core` Policies

The following are already owned by `job_core`:

```text
NoSerialize
NoReset
```

`job_schema` should consume these annotations through reflection but must not redefine them.

Their meaning remains whatever `job_core` defines.

This produces the policy ownership model:

```text
job_schema
    RW
    RO
    Const
    Required

job_core
    NoSerialize
    NoReset
```

The library that owns an operation should enforce the relevant policy while performing that operation.

For example:

```text
job_core serialization
    enforces NoSerialize

job_core reset
    enforces NoReset

job_core parsing
    enforces Required
```

while:

```text
job_schema_gen
    uses RW / RO / Const
    to determine emitted API
```

---

# Preliminary Member-Policy Matrix

The exact matrix must be verified during implementation, but the intended direction is:

| Policy            | Struct                         | Packed                               | BaseObject                                  | LightObject                               | Object                                              |
| ----------------- | ------------------------------ | ------------------------------------ | ------------------------------------------- | ----------------------------------------- | --------------------------------------------------- |
| No API annotation | plain member                   | packed member                        | reflected member, no generated property API | runtime member, no generated property API | reflected/runtime member, no generated property API |
| `RW`              | normally unnecessary           | invalid — Packed has no accessor API | getter + public setter                      | getter + public setter + Signal           | getter + public setter + Signal                     |
| `RO`              | normally unnecessary           | invalid — Packed has no accessor API | getter + internal setter                    | getter + internal setter + Signal         | getter + internal setter + Signal                   |
| `Const`           | normally unnecessary           | invalid — Packed has no accessor API | getter-only generated API                   | getter-only generated API                 | getter-only generated API                           |
| `Required`        | parser policy where applicable | normally irrelevant to raw layout    | enforced by persistence reader              | only where an input reader exists         | enforced by persistence reader                      |
| `NoSerialize`     | not normally applicable        | not applicable                       | honored                                     | not applicable                            | honored                                             |
| `NoReset`         | not normally applicable        | not applicable                       | honored                                     | not normally applicable                   | honored                                             |

This table is deliberately design-level rather than an API guarantee.

Remaining policy questions include:

```text
whether Struct should reject API annotations or merely ignore them
whether LightObject schemas ever participate directly in parsing
whether Const suppresses generated reset
exact visibility of RO mutation APIs
```

---

# Annotation Combination Rules

Some annotation combinations should be invalid.

Likely invalid combinations include:

```text
RW + RO
RW + Const
RO + Const
```

Other combinations need semantic consideration rather than a blanket prohibition.

For example:

```text
Required + Const
```

is perfectly meaningful:

> Must be supplied during reconstruction, but no generated mutation API exists afterward.

By contrast:

```text
Required + NoSerialize
```

is likely contradictory for a persistence-oriented object because a field cannot simultaneously be required from persistence input and excluded from persistence.

These rules belong in compile-time validation rather than scattered generator conditionals.

---

# Reflection Requirements

`job_schema` needs reusable reflection helpers capable of inspecting schema declarations at compile time.

The implementation will likely need to answer questions such as:

```text
What schema kind is this declaration?

What members exist?

What is each member's name?

What is each member's type?

What annotations are attached?

Does the member have a particular annotation type?

Is the type an array?

What is its extent?

Is it another schema type?

Is it a supported container?

Does it derive from a JOB object family?

Is it physically safe for Packed generation?
```

Where possible, these helpers should build on reflection utilities already present in `job_core`.

The schema library should not duplicate generic reflection traversal logic merely because generation is a new consumer.

---

# Compile-Time Validation Requirements

`job_schema` is responsible for JOB-specific compile-time schema validation.

Validation should be expressed through normal C++ mechanisms such as:

```text
concepts
consteval helpers
static_assert
reflection queries
contracts where appropriate
```

The preference is for failures to occur as close to the schema declaration as practical.

Likely validation categories include:

```text
exactly one schema kind
legal annotation combinations
legal annotations for containing kind
Packed-compatible member types
unsupported ownership types
nested schema compatibility
required generator assumptions
object-family requirements
```

The compiler error should identify the actual schema/member involved where the implementation permits useful diagnostics.

---

# Validation Model

The schema system should keep three forms of validation separate.

## Input / Presence Validation

Handled during parsing.

Examples:

```text
Required
missing fields
malformed representation
unsupported serialized input
```

This is recoverable input error handling.

---

## Domain Validation

Handled by application code.

Examples:

```text
manifest semantic rules
invalid field combinations
field-path diagnostics
multiple human-readable errors
configuration correctness
```

A domain validator may return many errors at once.

Bad user input is not a programmer error.

---

## Programmer / Runtime Invariants

Handled through C++ contracts.

Examples:

```text
preconditions
postconditions
internal state assumptions
programmer errors
runtime invariants
```

A contract violation represents broken program assumptions, not an invalid user configuration.

This distinction is why a universal `isValid()` should not be part of the generic JOB object model.

---

# Generation-Facing Requirements

Although `job_schema` does not generate files, it must expose enough compile-time functionality for `job_schema_gen` to generate code without rebuilding schema semantics independently.

The generation side should be able to ask questions such as:

```text
What kind is this schema?

What members should produce API?

Which members are RW / RO / Const?

Which members participate in persistence?

Which members are Required?

Which members may reset?

Is a member legal for Packed?

What JOB object family should be emitted?
```

Those answers should come from reusable `job_schema` facilities.

Generation code should not contain a parallel rule system such as:

```cpp
if (annotationName == "RO") {
    ...
}
```

where the annotation semantics can instead be derived through `job_schema`.

---

# Generation Support Ownership

Some generation support may need to be instantiated inside the temporary schema compilation in order to work directly with C++26 reflection.

That does **not** make it part of `libs/job_schema`.

Ownership remains:

```text
libs/job_schema
    schema semantics
    reflection helpers
    compile-time validation

apps/job_schema_gen
    source-emission machinery
    generator-facing templates
    code-writing support
    compiler/process orchestration
```

If emitter-support templates must be visible to the target-side helper, they may physically compile into that temporary helper.

They still conceptually belong to `job_schema_gen`.

Compilation location and architectural ownership are not the same thing.

`libs/job_schema` must remain usable without bringing in:

```text
job_io
compiler process management
temporary filesystem machinery
source emitters
```

---

# Schema Kind Declaration

The exact syntax used to identify:

```text
Struct
Packed
BaseObject
LightObject
Object
```

remains subject to the C++26 annotation/reflection implementation used by the compiler.

A likely underlying form is:

```cpp
[[=job::schema::Object{}]]
struct Device {
    ...
};
```

The important requirement is that kind selection lives in the C++ schema declaration rather than being repeated in CMake.

CMake should not need:

```cmake
TYPE OBJECT
```

if the C++ declaration already contains that information.

This keeps the C++ source authoritative.

Public shorthand may later provide a more compact declaration syntax, but that shorthand must map back to the same reflected policy.

---

# Gating C++26 Reflection Implementation — Schema Discovery

Schema discovery is not merely another open policy question.

It is a gating implementation problem for the generator architecture.

A schema input file may contain one or more schema declarations.

Before the `job_schema_gen` and CMake interfaces are frozen, the implementation must prove how supplied schema headers expose their schema roots to the temporary generation compilation.

The ideal flow is:

```text
schema headers
    ↓
C++26 reflection implementation
    ↓
discover schema declarations
    ↓
generate
```

without requiring CMake to repeat fully qualified C++ type names.

Possible mechanisms include:

```text
reflection-based declaration discovery
explicit compile-time registration
a C++ schema type list
one schema root per source unit
```

The preferred solution should keep schema identity inside C++.

However, the correct mechanism depends on what the available C++26 reflection implementation can actually express.

GCC currently leads this implementation work and is the compiler being used by JOB during development, but the design requirement is not GCC-specific.

The requirement is:

> **JOB Schema should use the capabilities of a conforming and sufficiently complete C++26 reflection implementation rather than designing around a permanent GCC-only schema model.**

Compiler-specific implementation details may be necessary while the standard feature set is still emerging.

They should remain isolated enough that another compiler with suitable C++26 reflection support can implement the same design later.

---

# Why Schema Discovery Is Gating

Most other open items have straightforward fallbacks.

For example:

```text
RO setter visibility
Const reset behavior
Packed ordering heuristic
```

can be changed without redesigning the build architecture.

Schema discovery is different.

It determines the boundary between:

```text
C++
CMake
job_schema_gen
```

and therefore affects the public generation interface.

For example, these are materially different APIs:

```text
SCHEMA_FILES foo.h
```

versus:

```text
SCHEMA_FILES foo.h
SCHEMA_TYPES job::foo::Foo
```

versus a required C++ registry inside `foo.h`.

Therefore:

> **Schema discovery must be experimentally proven before the generator/CMake interface is considered stable.**

This is one of the first implementation experiments for the project.

---

# Defaults and Reset Behavior

C++ member initialization should remain authoritative for default state.

The schema should not require a second declaration such as:

```text
default: 5000
```

when the C++ member already contains:

```cpp
std::uint32_t timeout{5000};
```

Generated reset behavior should derive from the C++ initialization model.

The unresolved question is not whether C++ remains authoritative.

It does.

The unresolved question is:

> How does generation materialize that default into the final emitted implementation?

Possible strategies include:

```text
reflection directly reproducing the initializer
reading values from a default-constructed schema input
generated per-member static default helpers
another mechanically generated C++ mechanism
```

A generated implementation similar in spirit to the historical Qt property approach:

```cpp
static const Type &default_timeout();
```

may ultimately be reasonable if the compiler cannot expose the initializer in a directly reusable form.

What must not happen is:

```text
C++ member initializer
+
separately maintained schema default
```

The requirement is:

> **Default values must have one source of truth: the C++ schema declaration.**

---

# Signals

Signals are generated behavior, not schema data.

For example:

```cpp
[[=job::schema::RW{}]]
std::string name{};
```

inside an `Object` or `LightObject` schema may result in a generated changed Signal.

The schema declaration should not normally need to separately write:

```cpp
Signal<const std::string &> nameChanged;
```

because that would duplicate the member policy.

Whether a changed Signal is generated depends on:

```text
schema kind
member API policy
final signal-generation matrix
```

`BaseObject` does not gain runtime Signal ownership merely because a member is `RW`.

An unannotated member receives no generated property Signal because it requested no generated property API.

`Const` receives no changed Signal because the generated API does not expose a mutation path.

---

# Type-Aware Generated APIs

Generated accessors should not mechanically use one signature shape for every C++ type.

For example, generation may reasonably choose different forms for:

```text
small scalar
enum
large value type
string
container
pointer-like type
```

The schema author should not have to annotate ordinary C++ calling-convention details.

Those rules should be centralized in schema/generation policy so every generated library follows the same JOB conventions.

---

# Contracts

Contracts belong primarily to `job_core` and generated runtime behavior.

`job_schema` may provide enough information for generated methods to carry appropriate contracts where those contracts are universal and mechanically derivable.

However, generation must not invent application semantics.

For example, it may be reasonable to generate a postcondition around mechanically generated internal state changes if such a rule is universally true.

It is not reasonable to guess:

```text
width must be > 0
name must not be empty
count must be less than 64
```

unless the schema explicitly gains some future policy expressing that requirement.

The initial schema vocabulary should remain small.

Contracts protect:

```text
programmer assumptions
runtime invariants
preconditions
postconditions
```

They do not replace:

```text
Required
parser errors
domain validation
human-facing diagnostics
```

---

# Dependency Boundary

The initial dependency direction should remain:

```text
job_core
    ↑
job_schema
```

`job_schema` should not require:

```text
job_io
job_net
CMake
QEMU
compiler-driver implementation
filesystem generation utilities
```

Those dependencies belong further up the generation stack.

This keeps the backing library usable for compile-time schema reasoning without dragging the generator toolchain into normal JOB libraries.

---

# Cross-Library Integration Work

The following work is outside `libs/job_schema` itself but is required for the complete schema design.

## `job_core` — `Required`

This is the first and highest-priority integration item.

Persistence readers must eventually:

```text
inspect Required
recognize missing fields
accumulate missing-field failures
surface them through JOB error/logging facilities
```

The implementation should support hand-written `BaseObject` types as well as generated ones.

`Required` should therefore become meaningful to the existing reflection persistence machinery rather than existing only inside generated code.

---

## `job_core` — `isValid()`

The generic `Object` and `LightObject` abstractions should stop requiring a universal:

```cpp
bool isValid() const noexcept;
```

Corresponding concepts should stop requiring it as well.

Any base-class abstractness requirement should be preserved structurally rather than through a fake universal validation API.

---

## `job_core` — Existing Annotations

`NoSerialize` and `NoReset` remain authoritative and must continue to be usable by both:

```text
hand-written objects
generated objects
```

JOB Schema must consume these existing semantics rather than fork them.

---

# Product Requirements

The implementation of `libs/job_schema` should satisfy the following requirements.

1. A schema must be written as valid C++26.

2. Schema kinds must be expressed in C++, not duplicated in CMake configuration.

3. Member policy must be expressed through reflection-visible C++ annotations or an equivalent standard C++26 mechanism.

4. The initial supported kinds are exactly:

   ```text
   Struct
   Packed
   BaseObject
   LightObject
   Object
   ```

5. The initial schema-owned member policies are:

   ```text
   RW
   RO
   Const
   Required
   ```

6. `job_schema_annotations.h` should own the canonical annotation definitions or their eventual equivalent.

7. A convenience layer such as `job_schema_alias.h` may provide shorter stable public spellings around the underlying annotations.

8. Design documentation should continue to use the explicit annotation syntax where it improves clarity.

9. The convenience syntax may evolve before the first stable `0.1` interface, but stable public shorthand should not be changed casually afterward.

10. Existing `job_core` policies such as:

    ```text
    NoSerialize
    NoReset
    ```

    must be consumed rather than redefined.

11. `Required` means input presence, not value validity.

12. `job_schema` must expose `Required` in a form that `job_core` persistence readers can inspect.

13. Missing Required fields should be collectable in one parse operation rather than necessarily failing on the first missing field.

14. `Object` and `LightObject` schemas must not require a universal generated `isValid()` function.

15. Contracts must be used for programmer/runtime invariants, not ordinary user-input validation.

16. Domain validation remains application code.

17. An unannotated object member must not silently become `RW`.

18. No API annotation means no generated property/accessor API.

19. `Const` means generated getter-only API, not immutable persisted storage.

20. `RO` means externally read-only with an internal generated mutation path where appropriate.

21. `RW` means public generated read/write API.

22. `Packed` must reject API-policy annotations because Packed has no generated accessor/property API.

23. `Packed` output must remain suitable for direct physical representation and avoid behavioral object machinery.

24. Packed layout must be validated by the actual target compiler.

25. `job_schema` must not generate source files.

26. `job_schema` must not own compiler/process/CMake orchestration.

27. `job_schema` must not create a required secondary schema IR.

28. Generator semantics must come from reusable `job_schema` facilities rather than a duplicated policy implementation.

29. Generation/emitter machinery belongs to `job_schema_gen`, even when some support templates must compile inside a temporary target-side helper.

30. C++ member initialization remains the authoritative source for defaults.

31. Default materialization must not require separately maintained schema defaults.

32. Schema discovery must be proven against the chosen C++26 reflection implementation before the generator/CMake interface is frozen.

33. The schema-discovery architecture should not permanently depend on GCC-specific behavior if another compiler later provides suitable C++26 reflection support.

34. The implementation should favor compiler diagnostics and compile-time failures over late generator errors wherever practical.

---

# Decisions Already Made

The following decisions are considered part of the current architecture unless implementation evidence requires revisiting them.

```text
C++26 is the canonical schema language.

Reflection is used to derive structure.

The compiler is the first schema linter.

There are five schema kinds:
    Struct
    Packed
    BaseObject
    LightObject
    Object

Struct remains plain C++.

Packed is allowed to reorder and pad generated physical layout.

Packed has no generated property/accessor API.

BaseObject is persistence-oriented.

LightObject is runtime/signal-oriented without persistence.

Object combines runtime behavior with BaseObject persistence.

No API annotation means no generated property API.

Const means generated getter-only API.

RO means generated read API with internal mutation.

RW means generated public read/write API.

Required means presence in parseable input.

NoSerialize and NoReset remain owned by job_core.

Required enforcement belongs in job_core readers.

Missing Required fields should be accumulated where practical.

Universal isValid() is not part of the generic schema/object model.

Contracts handle programmer/runtime invariants.

Application/domain validation remains separate.

No canonical intermediate schema IR is introduced.

CMake does not own schema semantics.

Generation/emission machinery belongs to job_schema_gen.

C++ member initializers remain the default-value authority.

Schema discovery is a gating C++26 reflection implementation question.
```

---

# Gating Implementation Work

Some questions are ordinary policy choices.

Others determine whether the architecture itself is viable.

The following work should happen early.

## 1. Schema Discovery

Prove how one or more schema declarations are discovered from supplied C++ schema headers.

This determines the public boundary between:

```text
C++
job_schema_gen
CMake
```

and therefore gates the generator interface.

The experiment should use the available C++26 reflection implementation directly.

GCC is currently the practical implementation used by JOB and is expected to be the first compiler tested, but the architectural requirement is C++26 reflection capability rather than "GCC forever."

---

## 2. Annotation Reflection

Prove that the selected annotation representation can be:

```text
attached to types
attached to members
queried by annotation type
distinguished reliably
used from consteval/template reflection code
```

The shorthand spelling does not matter to the generator as long as it resolves to the same reflected annotation semantics.

---

## 3. Default Materialization

Prove which C++26 mechanisms are available for preserving schema member initializers in generated reset/default behavior.

Candidate implementations include:

```text
reflected initializer reproduction
default schema-instance inspection
generated static per-member defaults
```

One of these must preserve the schema declaration as the only authoritative default.

---

## 4. Packed Reflection / Layout

Prove the target compiler provides all information required to:

```text
classify Packed-safe members
derive alignment
derive final size
validate generated layout
```

The final emitted result must still be validated in the target compilation.

---

# Open Design Questions

The following remain intentionally open.

## `RO` Setter Visibility

Should generated internal setters be:

```text
protected
private
private with generated friend/access helper
```

?

---

## `Const` Reset Semantics

Does `Const` suppress a generated reset API completely?

Can persistence/reset internals still restore the schema default even though normal generated mutation is prohibited?

---

## Struct API Annotations

Should API annotations on `Struct` be:

```text
compile-time errors
ignored
allowed but meaningless
```

?

Rejecting them is likely cleaner, but the behavior should be intentional.

---

## LightObject Parsing

Does a `LightObject` schema ever participate directly in JSON/YAML-style parsing?

If so, how does `Required` apply when the object family itself has no persistence semantics?

---

## Packed Type Set

What exact member types are allowed in `Packed`?

Which nested structures are recursively Packed-compatible?

---

## Packed Ordering

What packing/reordering algorithm should be used?

Possible policies include:

```text
strict size optimization
alignment grouping
stable ordering within equal alignment
optional explicit user grouping
automatic versus manual Packed modes
```

The implementation should remain inspectable and deterministic.

---

## Abstract Base Behavior

After removing `isValid()`, what exact mechanism keeps `Object` and `LightObject` abstract if that remains desirable?

The choice must fit existing JOB constructor/factory conventions.

---

## Generated Contracts

Which contracts, if any, are universal enough to derive automatically from schema policy without guessing application semantics?

---

## Diagnostics

How much schema/member-specific detail can the chosen C++26 reflection implementation surface through compile-time failures?

How should schema-related compiler failures integrate with the normal `job_schema_gen` error output?

How should parse-time `Required` failures integrate with `job_logger` and the broader JOB error-reporting direction?

---

## Comments 

job_schema ignores and discards documentation comments. They are not part of the reflected schema contract and are not reproduced as generated schema metadata.

If some documentation tool can read the original source comments directly, great. That is a documentation-tool concern.

And if the generated library needs user-facing/API documentation, that should be written in the generated library's surrounding docs/application layer, where the type actually has context.

# Design Principle

The implementation should continually be tested against one question:

> **Is this information already present in the C++ type?**

If the answer is yes, `job_schema` should derive it rather than asking the developer to declare it again.

If the answer is no, and the information changes generated behavior, it may belong in schema policy.

A second useful question is:

> **Who still has the information required to enforce this rule?**

For example:

```text
RW / RO / Const
    generation knows enough
    → job_schema / job_schema_gen

Required
    parser knows presence
    → job_core reader

NoSerialize
    serializer owns the operation
    → job_core

runtime invariant
    operation knows the invariant
    → contract

domain correctness
    application knows the semantics
    → application validation
```

Those two questions keep ownership clear and prevent schema policy from leaking into every layer.

The goal is not to make `job_schema` clever for its own sake.

The goal is to make the generated code boring, consistent, and difficult to accidentally desynchronize from the declaration that created it.
