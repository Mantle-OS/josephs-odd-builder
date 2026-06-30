# Job Serializer

Message schema + runtime object + codegen.

This is split into:
- `job::serializer` (schema, runtime object, emitters, file IO)
- `job::serializer::msgpack` (msgpack backend + msgpack emitter)

Apps:
- `job_msg_gen` = schema -> generated C++

## Layout

- `libs/job_serializer/*`
  - schema model + field model
  - runtime object (dynamic message)
  - base serializer interface (yaml/json built in)
  - emitters (C++ codegen base)
  - reader/writer helpers

- `libs/job_serializer_msgpack/*`
  - msgpack runtime serializer (binary)
  - msgpack code emitter (adds pack/unpack to generated structs)
  - msgpack utils (packing/unpacking helpers)

- `apps/job_msg_gen/*.cpp`
  - CLI generator (reads schema files, writes C++ output)

- `apps/job_msg_gen_example/*`
  - example schema inputs (yaml/json)
  - example CMake usage

## Schema

Schema files describe a message:
- metadata: tag/version/unit/base/out names
- fields: name + type (+ optional key/required/comment/etc)

Field types are string-based:
- scalars: `str`, `i32/u32`, `i64/u64`
- binary: `bin`, `bin[N]`
- lists: `list<...>`
- structs: `struct` (with a referenced type)

Schemas load from YAML or JSON.

## Runtime object
Runtime messages use a dynamic container:
- map of field name -> value
- values cover: integers, strings, binary blobs, lists, and nested “struct maps”

Used by:
- yaml/json encode/decode
- msgpack encode/decode
- tools that want “generic message instances” without generated structs


## Serializers

Base serializer:
- YAML text encode/decode
- JSON text encode/decode

plugins:
- binary formats (msgpack is implemented)
- other binary formats later (flatbuffers placeholder)

## Codegen (emitters)
Emitters take a schema and generate C++:
- a struct with typed fields
- optional extra methods added by backend emitters

The base emitter handles:
- struct layout
- includes
- mapping schema field types -> C++ types

Plugins extend this by injecting format-specific code.

## MsgPack backend

- runtime serializer:
  - encodes runtime objects to msgpack bytes
  - decodes msgpack bytes back into runtime objects
  - uses field names as the keys in the msgpack map

- msgpack emitter:
  - generates msgpack pack/unpack helpers for the generated C++ struct
  - shared helpers live in msgpack util code

## Message generator app (`job_msg_gen`)

CLI tool that:
- reads one or more schema files (yaml/json)
- selects an emitter (currently msgpack)
- writes generated header + source to an output directory

The example schemas live under `apps/job_msg_gen_example/`.


## CMake helper: `job_msgpack_gen_lib`

`job_msgpack_gen_lib` is a CMake helper for projects that want to build a generated MessagePack schema library.

It is defined in:

```text
cmake/job_msgpack_gen_lib.cmake
```

The helper reads one or more schema files, runs `job_msg_gen`, generates C++ source/header files, and builds them into a static or shared library.

Example shape:

```cmake
job_msgpack_gen_lib(
    NAME my_schema
    VERSION 0.1
    DESCRIPTION "Generated schema library"
    SCHEMA_FILES
        schema/foo.yaml
        schema/bar.yaml
    DEPENDENCIES
        SomeOtherTarget
)
```

This creates a target named:

```text
my_schema_lib
```

The generated files are written under the current build directory in a subdirectory named after `NAME`.

The helper links the generated library against:

* `JosephsOddBuilder_Serializer`
* `JosephsOddBuilder_Serializer_MsgPack`
* `msgpack-cxx`
* `Boost::boost`
* `Threads::Threads`
* YAML support

Optional arguments:

* `BUILD_SHARED` — build the generated schema library as shared instead of static
* `DEPENDENCIES` — additional link targets
* `TARGET_EXTRA_INCLUDE` — extra include directories for the generated target

This helper is mainly used when a schema should become a normal C++ library target that can be linked by other JOB components.

