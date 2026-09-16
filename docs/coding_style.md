# Job's Guidlines, Coding Style and Conventions

The Number one rule on the job site is [Don’t panic!](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#in0-dont-panic)

This document describes the job of job's style for the JOB Framework.   

The goals:

* Code is readable at a glance.
* Intent is obvious from structure and naming.
* Diffs are easy to review.
* Future-you does not hate past-you.

This is a living document. It captures how the codebase "already" looks and how new code should be written.

---

## 1. General Philosophy

* Prefer clarity over cleverness.
* Code should be mostly self-explanatory: names and structure should carry most of the meaning.
* Comments explain **why**, not **what**, except in tricky algorithms.
* Use the language features (const, noexcept, [[nodiscard]], [[unlikely]], [[likely]], chrono, atomics, etc.) when they make intent clearer.
* Keep abstractions small and composable instead of building monolithic frameworks.
* Please try to follow the core guidlines from the iso team in c++

---

## 2. Layout and Braces

When reading code I visually look for immediate visual patterns (visual object orientation).  

This help's (me) to navigate an implementation at a glance.

Over the years, I have picked up a specific set of rules that maximize scannability,

Whether I am tracing execution flow or scanning the layout and memory orientation of a struct/class etc . 

By enforcing strict structure on block placement and minimizing brace noise, 
intent becomes inheritly obvious without parsing the lines word for word. 

I mean its objects orentiation anyways why not treat the visual look of the code with the same light ? 

I am (very) open to suggestions to make this visual repersentation of the code better.

### 2.1 Function / method definitions

Function braces go on their own line (Allman style):

```cpp
void doSomething()
{
    // ...
}

int JobThread::start()
{
    // ...
}
```

This makes it easy to visually scan files for function boundaries.

### 2.2 Inside function bodies

Inside a function, use K&R style for control blocks when there is more than 1 line. However when the curly braces are not needed we do not use them:

```cpp
if (x > y) {
    doSomething();
    return;
}

for (int i = 0; i < n; ++i)
    process(i);

```

### 2.3 Single-line statements

Single-line `if` / `return` without braces is allowed but should stay readable in diffs:

```cpp
if (!ok)
    return;
```

Prefer:

* Condition on one line.
* Action on the next line.

This keeps patch/diff noise low when the condition or the body changes. Avoid cramming complex conditions and actions on a single line.

---

## 3. Indentation and Alignment

- Use consistent indentation (follow the existing project settings).
- Inside classes, it’s acceptable to vertically align member declarations for readability:
  - Pick a “column start” for the member names (after the type).
  - Pad with spaces so types line up and names start in the same column.
  - Don’t obsess over pixel-perfect alignment; aim for quick visual scanning.

- If the name of a private member is so long that the line approaches ~80 characters,
  that’s a smell. Prefer introducing a `using` alias or rethinking the name.

Example:

```cpp
class Foo {
public:
    // ...
private:
    int         m_count{0};
    float       m_scale{1.0f};
    std::string m_name;
};
```

Do not fight the formatter to achieve “perfect” alignment, use it to improve readability, not as a hard rule.

---

## 4. Naming

### 4.1 Types

* Classes / structs / enums: `PascalCase`.

```cpp
class JobThreadPool;
struct JobSporadicDescriptor;
enum class DiskZone;
```

### 4.2 Variables, functions, methods

* Prefer `lowerCamelCase` for functions and variables:

```cpp
int  taskCount{};
void startWatcher();
bool isRunning() const;
```

### 4.3 Members

* Private data members Always use `m_` prefix: 
Why use the old m_ prefix. One glance in a implenmtation files tell's you there are the public members


```cpp
class JobThread {
private:
    std::atomic<bool> m_running{false};
    ThreadPool::Ptr   m_pool;
};
```

This makes it immediately clear what lives in a class vs. local scope.

### 4.4 Constants

* Use `kPrefix` for constants:

```cpp
constexpr int   kNumHammerThreads = 8;
constexpr float kEpsilon          = 1e-6f;
```

### 4.5 C vs C++

* For C APIs / C compatibility layers, `snake_case` is acceptable and preferred.
* For C++ code, use the conventions above (`PascalCase` for types, `lowerCamelCase` for functions/vars).

---

## 5. References, Pointers, and Qualifiers

### 5.1 Reference / pointer placement

References and pointers attach on the right side of the type:

```cpp
const std::string &name;
JobThread        *thread;
auto             &ref = something;
```

Avoid `T& name` / `T* name` mixed with `auto&`/`auto *` on the same project; keep the `&` / `*` visually near the type.


Good
```cpp
for (const auto &str : strList)
    doSomething(str);
```

### 5.2 const

Use `const` aggressively where it conveys intent:

* `const` on function parameters that are not modified.
* `const` member functions when they don’t mutate observable state.
* Prefer `const auto &` for range loops over containers.

```cpp
void logMessage(const std::string &msg);

int size() const
{
    return m_items.size();
}

for (const auto &item : items)
    process(item);
```

### 5.3 [[nodiscard]] and noexcept

* Use `[[nodiscard]]` when ignoring a result is almost always a bug:

```cpp
[[nodiscard]] StartResult start();
[[nodiscard]] bool        isRunning() const noexcept;
```

* Use `noexcept` for functions that are expected not to throw, especially in low-level or hot-path code.

---

## 6. Ownership and Smart Pointer Aliases

### 6.1 Class-local aliases

Every heap-allocated, shared-lifetime type should provide its canonical pointer aliases:

```cpp
class JobThreadPool {
public:
    using Ptr  = std::shared_ptr<JobThreadPool>;
    using WPtr = std::weak_ptr<JobThreadPool>;

    // ...
};
```

Use these aliases everywhere instead of spelling out `std::shared_ptr<Foo>` at call sites.

Benefits:

* Code at the use site is shorter and clearer: `JobThreadPool::Ptr` makes the ownership model explicit.
* If ownership changes later (`unique_ptr`, handles), only the aliases need to be updated.
* It standardizes the vocabulary across the codebase.

### 6.2 Non-owning vs owning

* `Ptr` -> owning `shared_ptr`.
* `WPtr` -> non-owning `weak_ptr`.
* Raw pointers (`T*`) are allowed for non-owning, internal links where lifetime is obvious and controlled elsewhere, but prefer `WPtr` when cross-component lifetime can be tricky.

---

## 7. Role Typedefs and Generic Components

Generic components should define semantic aliases for their template parameters and roles, instead of leaking raw templates everywhere.

Example: pipeline stages:

```cpp
template <typename In, typename Out>
class JobPipelineStage : public JobThreadActor<In> {
public:
    using InputType  = In;
    using OutputType = Out;

    using NextActor = JobThreadActor<Out>;
    using NextPtr   = std::shared_ptr<NextActor>;

    // ...
};
```

Helpers (such as `JobPipeline::connect`) then talk to these semantic roles:

```cpp
template <typename Head, typename Next, typename... Rest>
static void connect(std::shared_ptr<Head> head, std::shared_ptr<Next> next, Rest... rest)
{
    if (head && next) {
        using HeadOut = typename Head::OutputType;
        using NextIn  = typename Next::InputType;
        static_assert(std::is_same_v<HeadOut, NextIn>,
                      "JobPipeline::connect: incompatible stage types");

        head->addNext(next);
    }

    if constexpr (sizeof...(rest) > 0)
        connect(next, rest...);
}
```

This keeps public APIs readable and lets the compiler enforce compatibility.

---

## 8. Error Handling, Logging, and Exceptions

* Throw standard exceptions (`std::runtime_error`, etc.) from library code when needed.
* Use error enums / result types (`StartResult`, etc.) where exceptions are too heavy or undesirable.
* Log meaningful events:

  * Admission/rejection decisions in schedulers.
  * Graph / pipeline task failures.
  * Timeouts, unexpected states.

Log messages should be:

* Descriptive enough to debug runtime behavior.
* Not spammy in normal operation (tests are allowed to be noisier).

---

## 9. Concurrency and Threading

* Use `std::atomic` for shared mutable state across threads.
* Prefer memory orders explicitly when relevant (`std::memory_order_relaxed` for counters, stronger orders where required).
* Encapsulate concurrency in well-defined primitives:

  * `JobThread`, `ThreadPool`, schedulers, actors, pipelines, grids, integrators, etc.
* Avoid ad-hoc synchronization scattered through business logic. Build on the primitives.

---


## 10. Visibility, Linkage, and Namespace Boundaries

This section governs how code bridges boundaries—both compiler boundaries (Translation Units) and linker boundaries (DLLs/Shared Libraries). 

### 10.1 Linkage and Header Integrity

Headers are blueprints meant for sharing identical definitions across translation units. 
They must never pollute or fragment the global space, nor trigger duplicate storage states.

*   **Banned: Anonymous Namespaces** 
Never place an anonymous (unnamed) namespace inside a header (OR ANYWHERE) file (`Rule SF.22`). 
It forces internal linkage, creating isolated duplicate copies of objects and functions in every translation unit that includes it, 
leading to hidden ODR (One Definition Rule) violations and massive code bloat.
*   **Banned: `static` Global Variables in Headers.** Similar to anonymous namespaces, marking a non-templated variable as `static` in a header creates a unique instance per translation unit. 
*   **Preferred: Modern External Linkage.** For header-only utilities, constants, 
and generated lookup tables, use `inline constexpr` (variables) or `inline static` (methods). 
This explicitly instructs the linker to merge all instances into exactly one global allocation across the final binary.

If you are confused you can [also see Cpp Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)

## 11. Tests

JOB uses Catch2 version 3 for unit tests and benchmarks.

Tests serve two purposes:

1. verify behavior; and
2. document how an object is intended to be used.

The detailed testing, validation, benchmarking, debugging, and accreditation workflow is defined in the [JOB Work Instructions](work_instructions/work_instructions.md) and the associated testing process documentation.

For new work, these rules apply beginning with JOB pre-alpha **0.0.0007**.

Tests written before `0.0.0007` may use the previous three-block testing structure. Existing tests do not need to be rewritten solely to conform to the newer process. Migration should occur naturally when older tests or modules are substantially modified.

### 11.1 Validation Structure

Unit tests are structured around two conceptual blocks.

#### Block One — Usage / Examples

Show how the object is intended to be used.

Ask:

> How should a developer use this object correctly?

Prefer small, realistic scenarios over artificial examples.

Tests in this block should act as executable documentation where practical so that JOB does not need to maintain a separate example suite for every object.

Examples include:

* constructing and configuring an object;
* performing the expected operation;
* demonstrating normal ownership and lifetime;
* showing realistic interaction between related objects; and
* validating expected output or state.

#### Block Two — Edge Cases / Breakage

Attempt to break the object.

Ask:

> What happens at the boundaries of the object's contract?

Examples include:

* empty inputs;
* zero sizes;
* null or missing resources where permitted;
* timeouts;
* unusual parameters;
* minimum and maximum values;
* invalid states;
* partial failures;
* `workerCount == 0`;
* empty graphs;
* `0x0` grids; and
* other behavior that protects invariants or catches regressions.

These tests may be more mechanical or verbose than the usage block.

### 11.2 Not Every Type Requires a Dedicated Test

Do not create a test file merely because a type exists.

Simple enums, passive data representations, and structs/classes containing only a few data members may be exercised naturally through the behavioral objects that consume them.

A dedicated test is warranted when an object has behavior, ownership, invariants, transformations, algorithms, state transitions, resource management, or another meaningful **job** that deserves independent validation.

See [What Is a JOB?](what_is_a_job.md) for the project terminology surrounding jobs and object responsibilities.

### 11.3 Test File Naming

Tests for an object should normally use:

```text
test_<object>.cpp
```

Test files are added to the test module's CMake `TESTS` collection.

Tests should validate one meaningful object at a time where practical.

### 11.4 Benchmarks Are Separate From Unit-Test Validation

Benchmarks are no longer the third validation block.

Benchmarking is a separate responsibility that occurs only after the object's unit-test validation has passed.

When benchmarking is warranted, use:

```text
benchmark_<object>.cpp
```

and add the file to the CMake `BENCHMARKS` collection.

Benchmarks are intended to characterize the released implementation, including:

* execution time;
* throughput;
* scaling;
* saturation;
* latency;
* stress behavior; and
* other relevant performance characteristics.

Benchmarks are only considered authoritative when run under the supported Release benchmark configuration.

Running benchmarks in Debug, RelWithDebInfo, or another unsupported configuration may be useful for private investigation, but resulting numbers are not considered a source of truth.

### 11.5 Benchmark Guards

Benchmark code must remain protected by `JOB_TEST_BENCHMARKS`.

Example:

```cpp
#ifdef JOB_TEST_BENCHMARKS
    #include <catch2/benchmark/catch_benchmark.hpp>
#endif
```

Benchmark test cases must also be guarded:

```cpp
#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("SomeObject benchmarks", "[module][some_object][benchmark]")
{
    // ...
}

#endif
```

### 11.6 Every Independently Meaningful Benchmark Must Use a Catch2 SECTION

Benchmarks that may need to be run or diagnosed independently **must** be wrapped in their own Catch2 `SECTION`.

This is required.

Do not place multiple independently meaningful benchmarks into one generic section.

Preferred:

```cpp
#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("UTF-8 validation benchmarks", "[core][utf8][benchmark]")
{
    SECTION("scalar validation")
    {
        BENCHMARK("scalar validation") {
            // ...
        };
    }

    SECTION("SIMD validation")
    {
        BENCHMARK("SIMD validation") {
            // ...
        };
    }

    SECTION("ASCII validation")
    {
        BENCHMARK("ASCII validation") {
            // ...
        };
    }
}

#endif
```

Avoid:

```cpp
SECTION("benchmarks")
{
    BENCHMARK("scalar") {
        // ...
    };

    BENCHMARK("SIMD") {
        // ...
    };
}
```

The section boundary allows the testing and debugging process to isolate a specific benchmark when investigating an anomaly.

See the testing work instructions for the diagnostic process and process-runner usage.

### 11.7 Test Names and Tags

Use Catch2 test names and tags that describe behavior.

Example:

```cpp
TEST_CASE("JobThreadGraph fan-in waits for all prerequisites", "[threading][graph][fanin]")
{
    // ...
}
```

Prefer names such as:

```text
fan-in waits for all prerequisites
partial failure blocks only dependents
Game of Life blinker oscillates
```

over generic names such as:

```text
test graph
test case 2
basic test
```

### 11.8 Realistic Scenarios

Prefer realistic mini-scenarios when possible.

Tests should normally cover:

* expected usage;
* happy paths;
* edge conditions;
* failure behavior;
* invariant protection; and
* regression cases discovered during development.

It is acceptable for test code to contain more narrative comments than production code when those comments improve understanding of the scenario.

Tests still follow the project-wide rules for naming, intent, comments, and documentation.

Before adding comments merely to explain test code, review:

* [Section 12 — Comments and Naming for Intent](#12-comments-and-naming-for-intent)
* [Section 13 — Documentation, Evolution, and Inline Comments](#13-documentation-evolution-and-inline-comments)

Those sections define when comments add useful meaning, when names should carry the intent instead, and how JOB treats inline documentation while APIs are still evolving.

The testing process does not override those rules.

### 11.9 CI/CD

`JOB_CI_BUILD` indicates that tests are being built or executed as part of JOB CI/CD infrastructure.

Example:

```cpp
#ifdef JOB_CI_BUILD
// Running under JOB CI/CD.
#endif
```

Use this guard for tests or workloads that should not execute on automated builders, such as:

* hardware saturation workloads;
* extremely long-running stress tests;
* environment-specific tests;
* benchmark workloads whose results are meaningful only on controlled physical hardware.

The detailed rules for CI/CD remain outside this section.

### 11.10 Common Test Tools

JOB provides common test-support utilities where appropriate.

Current examples include:

* `tests-fast-math-workaround.h`
  Provides test-side workarounds required because JOB is compiled with fast-math behavior.

* `transient_test_file.h`
  Provides temporary-file support for libraries that sit below `job_io` and therefore cannot depend on it directly. Examples include `job_core`, `job_threads`, and other low-level libraries that need temporary file I/O during testing.

* `test_spin_till.h`
  Provides common spin-until behavior for tests that need to wait for a condition.

Use existing common test infrastructure before creating another local implementation of the same behavior.

### 11.11 Fixtures

Each test module should maintain a shared fixture header.

The preferred name is:

```text
<module>_fixtures.h
```

Fixtures contain test-specific data structures, shared literal data, reusable fixture state, and other representations required to exercise the module.

Keep fixture definitions out of individual test files when they are shared or when centralizing them improves clarity.

Fixtures are reviewed during the testing process to determine whether anything has accidentally evolved into reusable production functionality.

### 11.12 Test Utilities

Each test module should maintain a shared utility header.

The preferred name is:

```text
<module>_utils.h
```

Test-only helper functions should live here rather than being repeatedly recreated as anonymous or local helper functions across different test files.

Utilities may begin as:

* one-off test helpers;
* shared setup functions;
* synthetic data constructors;
* benchmark fixture helpers; or
* other support behavior used by the test executable.

Repeated use is evidence.

When a utility reaches a fourth genuine use, review whether it still belongs in test infrastructure.

The fourth use does not automatically require promotion into the production library. Test-specific behavior may legitimately remain test-only regardless of reuse.

However, a repeatedly reused helper is a strong production-API candidate when:

* its behavior is generic;
* it is not inherently tied to Catch2, benchmarking, assertions, fixture construction, or test-process behavior; and
* the functionality would still make sense if the test executable did not exist.

The testing work instructions define the review and accreditation process for these candidates.

### 11.13 Namespaces

Test files should use the library namespace with a nested `tests` namespace.

Example:

```cpp
namespace job::usb::tests {

// ...

} // namespace job::usb::tests
```

Do not use broad file-scope `using namespace ...` directives as a substitute for the proper test namespace.

### 11.14 Validation Before Benchmarking

An object must pass its required validation before authoritative benchmarking begins.

Compilation alone is not validation.

Green assertions alone are not sufficient when behavior remains unexplained.

If tests fail, expose questionable behavior, or reveal a poor representation or contract, resolve those issues before moving to benchmarking.

Debug, RelWithDebInfo, IDE debugging, GDB, tracing, or other diagnostic configurations may be used while resolving validation problems.

After diagnosis, the object must return to the supported validation configuration and pass again before progressing.

### 11.15 Review Benchmark Results

A benchmark producing a number does not make that number correct.

Benchmark results must be reviewed for plausibility.

Investigate:

* unexpectedly poor results;
* unexpectedly good results;
* unexplained variance;
* suspiciously stable values;
* unexpected scaling;
* strange throughput;
* fixed-duration plateaus;
* performance inconsistent with the implementation; and
* anything else that appears anomalous.

There are no stupid questions during benchmark review.

Independent review from another developer or an AI agent is encouraged when a result is not understood.

Do not accept a benchmark result merely because it is favorable.

### 11.16 Scope Discipline

Testing and review should remain focused on the active object and active process step.

A good engineer can find problems almost anywhere.

Do not turn unrelated observations into active cleanup, redesign, refactoring, or optimization work unless the current task requires it or the developer explicitly expands the scope.

When a genuine blocker depends on an unclear contract or lower-level assumption, stop and resolve that misunderstanding rather than guessing.

### 11.17 Detailed Testing Process

This section defines the coding-style requirements for tests and benchmarks.

The complete testing lifecycle is defined by the JOB work instructions:

* [JOB Work Instructions](work_instructions/work_instructions.md)
* [Testing Process Steps](work_instructions/testing_process_steps.md)
* [Testing and Debugging](work_instructions/testing_debug.md)
* [Testing AI Context](work_instructions/testing_ai_ctx.md)
* [Real-Time Testing](work_instructions/testing_rt.md)

The work instructions define the process gates, diagnostic workflow, benchmark review, fixture and utility accreditation, module completion, and handoff to later versioning or CI/CD processes.

---


## 12. Comments and Naming for Intent

* Names should carry meaning: prefer `count` / `index` / `deadlineMs` over `c` / `i` / `d`.
* Shorthand like `cnt` is acceptable when idiomatic and obvious; single letters are best kept for local loop indices or math.
* Comments should be used when:

  * Explaining non-obvious math or algorithms (e.g. Barnes–Hut, Verlet, sporadic EDF logic).
  * Explaining *why* a design choice was made, not restating the obvious.

Example:

```cpp
// Submit *fewer* tasks than there are threads to force stealing.
const int numTasks = 2;
```

---

## 13. Documentation, Evolution, and Inline Comments

Documentation should describe the current architecture and intended usage,
not every implementation detail.

It’s acceptable to delay heavy prose until APIs stabilize; in the meantime:

- Keep headers clean.
- Keep tests expressive and realistic.
- Use short file-level or type-level comments to capture intent.

As the libraries solidify, this file and `threading_overview.md` should be
updated to match reality.

### Inline comments

Most importantly: **have fun with inline comments, be clever make the user(reader) think and have a memory to go back too.**

Comments are encouraged to be:
- A bit weird.
- Occasionally snarky.
- Lightly narrative.
- Hold double-meaning, as long as they’re not confusing.
- Capable of making the reader laugh or at least smirk.

Examples of “on-brand” comments:

```cpp
// Late to the party ?
// Who's the boss? Tony Danza.
// Highway to the danger zone Goose ...
// Euler is Rk4's drunk cousin 
```

The only hard rule:

__A comment should add meaning that is not already obvious from the code. And should make the user reading it chuckle or think__

- If the comment is just narrating what each line does, it’s noise.

Prefer comments that explain:
- Why we’re doing this.
- What invariant we rely on.
- What assumption future changes must not break.
- Or a short mental model / metaphor that makes the block easier to hold in your head.
- [This kinda thing is allowed](https://youtu.be/g4uOsS0FsEs?si=qRXsYnxzYA36HGuy) 

If you deleted the comments and nothing of meaning was lost, they were probably unnecessary.

Discouraged patterns:
```cpp
// 1. step one
// 2. something boring
// 3. something that the code explains already
```
