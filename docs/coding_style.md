# Coding Style and Conventions

This document describes the house style for the `job_*` libraries, tests and applications.

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
* Use the language features (const, noexcept, [[nodiscard]], chrono, atomics, etc.) when they make intent clearer.
* Keep abstractions small and composable instead of building monolithic frameworks.

---

## 2. Layout and Braces

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

* Private data members use `m_` prefix:

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
for (cosnt auto &str : strList)
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

## 10. Tests

We use Catch2 (version 3) for tests

Tests serve as both verification and documentation. 

We generally structure tests in **three conceptual blocks**. Some older tests don’t follow this yet..., but this is the direction:

- **Block one: usage / examples**
  - “How do I use this class or struct?”
  - Real-world usage as tests. The goal is for tests to *also* be examples, so we don’t have to maintain a separate “examples” section for most things.
  - Prefer small, realistic scenarios over synthetic ones.

- **Block two: edge cases**
  - Stress the corners: empty inputs, zero sizes, null pools, timeouts, weird parameters, “workerCount = 0”, etc.
  - These exist mainly to catch regressions and guard invariants.
  - It’s fine if these are a bit more mechanical or verbose.

- **Block three: benchmarks / stress if needed (they are not all the time)**
  - [Push it to the limit](https://youtu.be/3-3Yok5D3Aw?si=S7LgO8yxn0OCCdMK)
  - Micro-benchmarks (e.g. thread startup latency) and stress tests (e.g. 10k tasks through a scheduler, high-volume actor mailboxes).
  - These tests are about performance characteristics and stability, not correctness alone.

* Use Catch2 test names and tags that describe behavior:

```cpp
TEST_CASE("JobThreadGraph fan-in waits for all prerequisites",
          "[threading][graph][fanin]")
{ /* ... */ }
```

* Prefer realistic mini-scenarios:
  * E.g. “fan-in waits for all prerequisites”, “partial failure blocks only dependents”, “Game of Life blinker oscillates”.

* Cover:
  * Happy paths.
  * Edge cases (empty graphs, 0x0 grids, workerCount = 0, etc.).
  * Failure behavior (exceptions, rejection, timeouts).

* It’s acceptable for tests to contain more narrative comments than production code.

---

## 11. Comments and Naming for Intent

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

## 12. Documentation, Evolution, and Inline Comments

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
