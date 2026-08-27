# QLlama

QLlama is the core runtime layer of the QtAI stack. It provides a full C++/Qt binding over `llama.cpp`, exposing model loading, context execution, sampling, tokenization, and streaming inference to both Qt C++ and QML.

It is designed as a *low-level AI execution engine*, not a framework or orchestration system.

Higher-level concerns such as:
- user sessions
- authentication
- package management
- model distribution
- multi-user routing

are intentionally excluded and will be handled by **QSession** and related systems.

---

## Architecture Overview

QLlama is structured as a layered execution pipeline:

```text

QLlamaModel
↓
QLlamaContext
↓
QLlamaSampler
↓
QLlamaEngine (abstract)
├── QLlamaLocalEngine
├── QLlamaClientEngine
└── QLlamaServerEngine

````


Each layer has a strict responsibility boundary.

---
Ah yes—that actually changes the philosophy of the library, and I think it's one of the strongest design decisions in QtAI. I would make it a dedicated section rather than just mentioning it in `QLlamaBase`.

I'd add something like this after the architecture section.


## Configuration and Serialization

Nearly every major runtime object in QLlama inherits from `QLlamaBase`.

This provides a common serialization interface across the entire runtime:

- JSON serialization
- YAML serialization
- Loading configuration from disk
- Saving configuration to disk
- Runtime inspection and debugging

Every major configuration object can therefore be treated as both a live runtime object and a persistent configuration document.

For example:

- `QLlamaModelParams`
- `QLlamaContextParams`
- `QLlamaSampler`
- `QLlamaModelKvOverride`
- `QLlamaAdapterLora`

all inherit from `QLlamaBase`.

This allows an entire inference configuration to be saved before execution and reproduced later.

Example:

```cpp
sampler->saveToJsonFile("sampler.json");
contextParams->saveToYamlFile("context.yaml");
modelParams->saveToJsonFile("model.json");
```

Likewise, configurations can be restored:

```cpp
sampler->loadFromJsonFile("sampler.json");
```

Its kinda like this
```
          JSON
             ▲
             │
          YAML
             ▲
             │
QLlamaBase object
      │
      ├── Runtime state
      ├── Qt properties
      ├── Serialization
      ├── Debugging
      └── Native llama.cpp structure
```

## Core Design Goals

QLlama is built around the following principles:

- Thin C++ wrapper over `llama.cpp`
- Full QML exposure via `QML_ELEMENT`
- Deterministic ownership of native pointers
- Explicit lifecycle control (no hidden global state)
- Streaming-first inference design
- Separation of model / context / sampling logic

---

## Model Layer

### QLlamaModel

Responsible for loading and unloading `.gguf` models.

Key responsibilities:
- Load model weights from disk
- Manage `llama_model*`
- Expose model metadata (architecture, tensor count, parameter count)
- Provide access to vocabulary

Example:

```cpp
QLlamaModel model;
model.set_modelPath("model.gguf");
model.loadModel(params);
````

---

## Context Layer

### QLlamaContext

Represents an active execution graph for inference.

Key responsibilities:

* Own `llama_context*`
* Manage KV cache lifecycle
* Track token position state
* Apply LoRA adapters
* Perform decoding (`llama_decode`)

Important notes:

* Context is stateful
* KV cache is mutable and shared across inference steps
* Context must be explicitly reset or recreated for isolation

---

## Sampling Layer

### QLlamaSampler

Builds a dynamic sampling pipeline for token selection.

Supports:

* Top-K
* Top-P (nucleus sampling)
* Min-P filtering
* Temperature scaling
* XTC sampling
* Mirostat v1 / v2
* Greedy mode

Internally builds a `llama_sampler_chain`.

Sampling is lazily compiled and cached until parameters change.

---

## Engine Layer

### QLlamaEngine (abstract)

Defines the streaming inference interface:

```cpp
virtual void generate() = 0;
virtual void cancel() = 0;
```

Signals:

* `generationStarted()`
* `tokenStreamed(QString)`
* `generationFinished(QString)`
* `executionError(QString)`

---

### QLlamaLocalEngine

Runs inference directly against `llama.cpp` in a background thread.

Key features:
* Qt thread worker model
* Token-by-token streaming
* Direct context + sampler execution
* Low latency, no network overhead

---

### QLlamaClientEngine

HTTP client engine for remote inference servers.

Key features:

* OpenAI-compatible API support
* Streaming SSE parsing
* Request queueing
* Token-level emission via Qt signals

---

### QLlamaServerEngine

Implements a lightweight inference server.

Key features:

* Built on `QTcpServer`
* Manual HTTP parsing
* SSE streaming output
* Direct llama.cpp integration
* OpenAI-style endpoint compatibility

---

## LoRA Support

QLlama supports runtime LoRA adapters via:

### QLlamaAdapterLora

* Loads LoRA weights via `llama_adapter_lora_init`
* Applies scaling factor per adapter
* Can be attached to a context dynamically

Adapters are synchronized through:

```
QLlamaContext::syncActiveLoRAs()
```

---

## Vocabulary System

### QLlamaVocab

Provides token-level utilities:

* Tokenization
* Detokenization
* Token → string conversion
* BOS / EOS / NL token access

Backed by `llama_vocab*`.

---

## KV Overrides

### QLlamaModelKvOverride

Allows low-level model parameter overrides at load time.

Used for:

* experimental model configs
* backend tuning
* advanced inference control

---

## Model Parameters

### QLlamaModelParams

Controls model loading behavior:

* GPU layer offload
* mmap / direct IO
* tensor backend overrides
* KV override injection

---

## Context Parameters

### QLlamaContextParams

Controls execution runtime:

* context size
* batch size
* thread count
* attention configuration
* RoPE scaling parameters
* KV cache behavior

---

## Sampling Configuration

Sampling behavior is fully configurable via QML or C++.

Example:

```qml
QLlamaSampler {
    temperature: 0.7
    topP: 0.9
    topK: 40
}
```

---

## Threading Model

* Local engine uses `QThread` worker model
* Client engine uses Qt Network async callbacks
* Server engine uses socket event loop

No blocking is performed on the UI thread.

---

## Memory Model

All native pointers are explicitly owned:

* `llama_model*` → QLlamaModel
* `llama_context*` → QLlamaContext
* `llama_sampler*` → QLlamaSampler
* LoRA adapters → QLlamaContext

No global llama state is assumed.

---

## Relationship to QSession (Important)

QLlama is NOT responsible for:

* authentication
* user identity
* token encryption
* model caching strategy
* multi-user routing
* package management

Those responsibilities belong to:

> QSession (future layer)

QLlama only provides execution primitives.

---

## Example (Local Inference)

```cpp
engine->set_model(model);
engine->set_context(context);
engine->set_sampler(sampler);

engine->set_prompt("Hello world");
engine->generate();
```

---

## Example (QML)

```qml
QLlamaLocalEngine {
    model: localModel
    context: localContext
    sampler: localSampler

    onTokenStreamed: console.log(chunk)
}
```

---

## Status

This is **alpha-level infrastructure**, but already provides:

* full llama.cpp binding
* streaming inference
* local + remote execution
* server mode
* QML integration

Future work will focus on:

* QSession integration
* multi-user execution model
* scheduler / queue system
* context isolation improvements
* performance tuning for KV cache reuse

```

