#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_ggml_context.h>
#include <job_ggml_enums.h>
#include <job_ggml_tensor.h>

#include <job_gguf_context.h>
#include <job_gguf_kv.h>

#include "test_ggml_utils.h"

using namespace job::ggml;

namespace {

void populateContext(JobGgufContext &context)
{
    context.setKeyValue(JobGgufKv{"general.architecture", std::string{"job-test"}});

    context.setKeyValue(JobGgufKv{
        "general.name",
        std::string{"JobGgufContext Test"}
    });

    context.setKeyValue(JobGgufKv{
        "job.context_length",
        std::uint32_t{4096}
    });

    context.setKeyValue(JobGgufKv{
        "job.scale",
        0.75f
    });

    context.setKeyValue(JobGgufKv{
        "job.enabled",
        true
    });

    context.setKeyValue(JobGgufKv{
        "job.dimensions",
        std::vector<std::uint32_t>{
            64,
            128,
            256
        }
    });

    context.setKeyValue(JobGgufKv{
        "job.labels",
        std::vector<std::string>{
            "context",
            "tensor",
            "gguf"
        }
    });
}

} // namespace


// Block one: usage / examples
TEST_CASE("GGUF context creates an empty native document", "[gguf][context][usage][construction]")
{
    JobGgufContext context;

    REQUIRE(context.isValid());
    REQUIRE(context.context() != nullptr);
    REQUIRE(context.version() > 0);
    REQUIRE(context.alignment() > 0);
    REQUIRE(context.keyValueCount() == 0);
    REQUIRE(context.tensorCount() == 0);
    REQUIRE(context.dataOffset() == 0);
    REQUIRE(context.metadataSize() > 0);
}

TEST_CASE("GGUF context exposes mutable and const native access", "[gguf][context][usage][native]"
    )
{
    JobGgufContext context;

    gguf_context *nativeContext = context.context();
    REQUIRE(nativeContext != nullptr);

    const JobGgufContext &constContext = context;
    const gguf_context *constNativeContext = constContext.context();

    REQUIRE(constNativeContext != nullptr);
    REQUIRE(constNativeContext == nativeContext);
}

TEST_CASE("GGUF context stores and retrieves scalar key values", "[gguf][context][usage][kv][scalar]")
{
    JobGgufContext context;

    context.setKeyValue(JobGgufKv{
        "job.u8",
        std::uint8_t{8}
    });

    context.setKeyValue(JobGgufKv{
        "job.i16",
        std::int16_t{-16}
    });

    context.setKeyValue(JobGgufKv{
        "job.u32",
        std::uint32_t{32}
    });

    context.setKeyValue(JobGgufKv{
        "job.i64",
        std::int64_t{-64}
    });

    context.setKeyValue(JobGgufKv{
        "job.f32",
        3.25f
    });

    context.setKeyValue(JobGgufKv{
        "job.f64",
        6.5
    });

    context.setKeyValue(JobGgufKv{
        "job.bool",
        true
    });

    context.setKeyValue(JobGgufKv{
        "job.string",
        std::string{"glasses"}
    });

    REQUIRE(context.keyValueCount() == 8);

    {
        auto value = context.keyValue("job.u8");
        REQUIRE(value != nullptr);
        REQUIRE(value->isScalar());
        REQUIRE(value->type() == JobGgufType::UInt8);
        REQUIRE(value->value<std::uint8_t>() == 8);
    }

    {
        auto value = context.keyValue("job.i16");

        REQUIRE(value != nullptr);
        REQUIRE(value->type() == JobGgufType::Int16);
        REQUIRE(value->value<std::int16_t>() == -16);
    }

    {
        auto value = context.keyValue("job.u32");

        REQUIRE(value != nullptr);
        REQUIRE(value->type() == JobGgufType::UInt32);
        REQUIRE(value->value<std::uint32_t>() == 32);
    }

    {
        auto value = context.keyValue("job.i64");
        REQUIRE(value != nullptr);
        REQUIRE(value->type() == JobGgufType::Int64);
        REQUIRE(value->value<std::int64_t>() == -64);
    }

    {
        auto value =context.keyValue("job.f32");
        REQUIRE(value != nullptr);
        REQUIRE(value->type() == JobGgufType::Float32);
        REQUIRE(value->value<float>() == 3.25f);
    }

    {
        auto value = context.keyValue("job.f64");

        REQUIRE(value != nullptr);
        REQUIRE(value->type() == JobGgufType::Float64);
        REQUIRE(value->value<double>() == 6.5);
    }

    {
        auto value = context.keyValue("job.bool");

        REQUIRE(value != nullptr);
        REQUIRE(value->isBoolean());
        REQUIRE(value->value<bool>());
    }

    {
        auto value = context.keyValue("job.string");

        REQUIRE(value != nullptr);
        REQUIRE(value->isString());
        REQUIRE(value->value<std::string>() == "glasses");
    }
}

TEST_CASE("GGUF context stores and retrieves array key values", "[gguf][context][usage][kv][array]")
{
    JobGgufContext context;

    const std::vector<std::uint32_t> dimensions {
        64,
        128,
        256,
        512
    };

    const std::vector<bool> enabled {
        true,
        false,
        true
    };

    const std::vector<std::string> labels {
        "alpha",
        "beta",
        "gamma"
    };

    context.setKeyValue(JobGgufKv{
        "job.dimensions",
        dimensions
    });

    context.setKeyValue(JobGgufKv{
        "job.enabled",
        enabled
    });

    context.setKeyValue(JobGgufKv{
        "job.labels",
        labels
    });

    REQUIRE(context.keyValueCount() == 3);

    {
        auto value = context.keyValue("job.dimensions");

        REQUIRE(value != nullptr);
        REQUIRE(value->isArray());
        REQUIRE(value->type() == JobGgufType::UInt32);
        REQUIRE(value->elementCount() == dimensions.size());
        REQUIRE(value->values<std::uint32_t>() == dimensions);
    }

    {
        auto value = context.keyValue("job.enabled");

        REQUIRE(value != nullptr);
        REQUIRE(value->isArray());
        REQUIRE(value->isBoolean());
        REQUIRE(value->values<bool>() == enabled);
    }

    {
        auto value =
            context.keyValue(
                "job.labels"
                );

        REQUIRE(value != nullptr);
        REQUIRE(value->isArray());
        REQUIRE(value->isString());
        REQUIRE(value->values<std::string>() == labels);
    }
}

TEST_CASE(
    "GGUF context exposes key lookup and type inspection",
    "[gguf][context][usage][kv][inspection]"
    )
{
    JobGgufContext context;

    populateContext(context);

    REQUIRE(context.hasKey("general.architecture"));
    REQUIRE(context.hasKey("job.dimensions"));

    const std::int64_t architectureIndex =
        context.keyIndex(
            "general.architecture"
            );

    const std::int64_t dimensionsIndex =
        context.keyIndex(
            "job.dimensions"
            );

    REQUIRE(architectureIndex >= 0);
    REQUIRE(dimensionsIndex >= 0);

    REQUIRE(
        context.key(
            architectureIndex
            ) == "general.architecture"
        );

    REQUIRE(
        context.valueType(
            architectureIndex
            ) == JobGgufType::String
        );

    REQUIRE(
        context.ggufValueType(
            architectureIndex
            ) == GGUF_TYPE_STRING
        );

    REQUIRE(
        context.valueType(
            dimensionsIndex
            ) == JobGgufType::Array
        );

    REQUIRE(
        context.ggufValueType(
            dimensionsIndex
            ) == GGUF_TYPE_ARRAY
        );

    REQUIRE(
        context.arrayElementType(
            dimensionsIndex
            ) == JobGgufType::UInt32
        );

    REQUIRE(
        context.ggufArrayElementType(
            dimensionsIndex
            ) == GGUF_TYPE_UINT32
        );

    REQUIRE(
        context.arrayElementType(
            architectureIndex
            ) == JobGgufType::String
        );
}

TEST_CASE("GGUF context enumerates all key values","[gguf][context][usage][kv][enumeration]")
{
    JobGgufContext context;
    populateContext(context);
    const auto values = context.keyValues();

    REQUIRE(values.size() == static_cast<std::size_t>(context.keyValueCount()));

    REQUIRE_FALSE(values.empty());

    for (const auto &value : values) {
        REQUIRE(value != nullptr);
        REQUIRE(value->isValid());
        REQUIRE_FALSE(value->key().empty());
    }
}

TEST_CASE(
    "GGUF context replaces an existing key value",
    "[gguf][context][usage][kv][replace]"
    )
{
    JobGgufContext context;

    context.setKeyValue(
        JobGgufKv{
            "job.value",
            std::uint32_t{10}
        }
        );

    REQUIRE(context.keyValueCount() == 1);

    context.setKeyValue(
        JobGgufKv{
            "job.value",
            std::uint32_t{20}
        }
        );

    REQUIRE(context.keyValueCount() == 1);

    auto value =
        context.keyValue(
            "job.value"
            );

    REQUIRE(value != nullptr);
    REQUIRE(value->value<std::uint32_t>() == 20);
}

TEST_CASE(
    "GGUF context copies key values from another context",
    "[gguf][context][usage][kv][copy]"
    )
{
    JobGgufContext source;
    JobGgufContext destination;

    populateContext(source);

    destination.setKeyValue(
        JobGgufKv{
            "destination.existing",
            std::uint32_t{1}
        }
        );

    destination.setKeyValues(source);

    REQUIRE(
        destination.keyValueCount() ==
        source.keyValueCount() + 1
        );

    REQUIRE(
        destination.hasKey(
            "destination.existing"
            )
        );

    REQUIRE(
        destination.hasKey(
            "general.architecture"
            )
        );

    REQUIRE(
        destination.hasKey(
            "job.labels"
            )
        );
}

TEST_CASE(
    "GGUF context removes key values",
    "[gguf][context][usage][kv][remove]"
    )
{
    JobGgufContext context;

    populateContext(context);

    const std::int64_t countBefore =
        context.keyValueCount();

    const std::int64_t removedIndex =
        context.removeKey(
            "job.scale"
            );

    REQUIRE(removedIndex >= 0);

    REQUIRE(
        context.keyValueCount() ==
        countBefore - 1
        );

    REQUIRE_FALSE(
        context.hasKey(
            "job.scale"
            )
        );

    REQUIRE(
        context.removeKey(
            "job.scale"
            ) == -1
        );
}

TEST_CASE(
    "GGUF context adds and inspects tensor descriptions",
    "[gguf][context][usage][tensor]"
    )
{
    constexpr std::int64_t elementCount = 8;

    constexpr std::size_t payloadBytes =
        static_cast<std::size_t>(
            elementCount
            ) * sizeof(float);

    auto tensorContext =
        createAllocatedHostContext(
            1,
            payloadBytes
            );

    REQUIRE(tensorContext != nullptr);
    REQUIRE(tensorContext->isValid());

    auto tensor =
        tensorContext->newTensor1d(
            JobGgmlType::F32,
            elementCount
            );

    REQUIRE(tensor != nullptr);
    REQUIRE(tensor->isValid());

    tensor->setName(
        "job.context.tensor"
        );

    REQUIRE(tensor->hasName());

    JobGgufContext context;

    context.addTensor(
        *tensor
        );

    REQUIRE(context.tensorCount() == 1);

    REQUIRE(
        context.hasTensor(
            "job.context.tensor"
            )
        );

    const std::int64_t tensorIndex =
        context.tensorIndex(
            "job.context.tensor"
            );

    REQUIRE(tensorIndex == 0);

    REQUIRE(
        context.tensorName(
            tensorIndex
            ) == "job.context.tensor"
        );

    REQUIRE(
        context.tensorType(
            tensorIndex
            ) == JobGgmlType::F32
        );

    REQUIRE(
        context.ggmlTensorType(
            tensorIndex
            ) == GGML_TYPE_F32
        );

    REQUIRE(
        context.tensorSize(
            tensorIndex
            ) == payloadBytes
        );

    REQUIRE(
        context.tensorOffset(
            tensorIndex
            ) == 0
        );
}

TEST_CASE(
    "GGUF context changes a stored tensor type",
    "[gguf][context][usage][tensor][type]"
    )
{
    constexpr std::int64_t elementCount = 32;

    constexpr std::size_t payloadBytes =
        static_cast<std::size_t>(
            elementCount
            ) * sizeof(float);

    auto tensorContext =
        createAllocatedHostContext(
            1,
            payloadBytes
            );

    REQUIRE(tensorContext != nullptr);

    auto tensor =
        tensorContext->newTensor1d(
            JobGgmlType::F32,
            elementCount
            );

    REQUIRE(tensor != nullptr);

    tensor->setName(
        "job.type.tensor"
        );

    JobGgufContext context;

    context.addTensor(
        *tensor
        );

    REQUIRE(
        context.tensorType(0) ==
        JobGgmlType::F32
        );

    context.setTensorType(
        "job.type.tensor",
        JobGgmlType::F16
        );

    REQUIRE(
        context.tensorType(0) ==
        JobGgmlType::F16
        );

    REQUIRE(
        context.ggmlTensorType(0) ==
        GGML_TYPE_F16
        );
}

TEST_CASE(
    "GGUF context stores tensor payload data",
    "[gguf][context][usage][tensor][data]"
    )
{
    constexpr std::int64_t elementCount = 8;

    constexpr std::size_t payloadBytes =
        static_cast<std::size_t>(
            elementCount
            ) * sizeof(float);

    auto tensorContext =
        createAllocatedHostContext(
            1,
            payloadBytes
            );

    REQUIRE(tensorContext != nullptr);

    auto tensor =
        tensorContext->newTensor1d(
            JobGgmlType::F32,
            elementCount
            );

    REQUIRE(tensor != nullptr);
    REQUIRE(tensor->data() != nullptr);

    tensor->setName(
        "job.data.tensor"
        );

    for (std::int64_t index = 0;
         index < elementCount;
         ++index) {
        tensor->data()->setValueF32(
            index,
            static_cast<float>(index) + 0.25f
            );
    }

    JobGgufContext context;

    context.addTensor(
        *tensor
        );

    context.setTensorData(
        tensor->name(),
        tensor->dataPointer()
        );

    REQUIRE(context.tensorCount() == 1);

    std::vector<std::byte> payload(
        payloadBytes
        );

    std::memcpy(
        payload.data(),
        tensor->dataPointer(),
        payload.size()
        );

    context.setTensorData(
        tensor->name(),
        payload
        );

    REQUIRE(context.tensorCount() == 1);
}

TEST_CASE(
    "GGUF context serializes document metadata",
    "[gguf][context][usage][metadata]"
    )
{
    JobGgufContext context;

    populateContext(context);

    const std::size_t metadataSize =
        context.metadataSize();

    REQUIRE(metadataSize > 0);

    const std::vector<std::byte> metadata =
        context.metadata();

    REQUIRE(
        metadata.size() ==
        metadataSize
        );

    REQUIRE_FALSE(metadata.empty());

    REQUIRE(
        metadata[0] ==
        std::byte{'G'}
        );

    REQUIRE(
        metadata[1] ==
        std::byte{'G'}
        );

    REQUIRE(
        metadata[2] ==
        std::byte{'U'}
        );

    REQUIRE(
        metadata[3] ==
        std::byte{'F'}
        );
}

// ============================================================================
// Block two: edge cases / contracts
// ============================================================================

TEST_CASE(
    "GGUF context rejects a null native context",
    "[gguf][context][edge][construction]"
    )
{
    REQUIRE_THROWS_AS(
        JobGgufContext{
            static_cast<gguf_context *>(nullptr)
        },
        std::invalid_argument
        );
}

TEST_CASE(
    "GGUF context reset releases the native document",
    "[gguf][context][edge][reset]"
    )
{
    JobGgufContext context;

    populateContext(context);

    REQUIRE(context.isValid());
    REQUIRE(context.keyValueCount() > 0);

    context.reset();

    REQUIRE_FALSE(context.isValid());
    REQUIRE(context.context() == nullptr);

    REQUIRE(context.version() == 0);
    REQUIRE(context.alignment() == 0);
    REQUIRE(context.dataOffset() == 0);
    REQUIRE(context.metadataSize() == 0);

    REQUIRE(context.keyValueCount() == 0);
    REQUIRE(context.tensorCount() == 0);
}

TEST_CASE(
    "GGUF context reset adopts a replacement native context",
    "[gguf][context][edge][reset][ownership]"
    )
{
    JobGgufContext context;

    populateContext(context);

    gguf_context *nativeBefore =
        context.context();

    REQUIRE(nativeBefore != nullptr);

    gguf_context *replacement =
        gguf_init_empty();

    REQUIRE(replacement != nullptr);
    REQUIRE(replacement != nativeBefore);

    context.reset(
        replacement
        );

    REQUIRE(context.isValid());

    REQUIRE(
        context.context() ==
        replacement
        );

    REQUIRE(context.keyValueCount() == 0);
    REQUIRE(context.tensorCount() == 0);
}

TEST_CASE(
    "GGUF context ignores resetting to the currently owned pointer",
    "[gguf][context][edge][reset][same]"
    )
{
    JobGgufContext context;

    populateContext(context);

    gguf_context *nativeContext =
        context.context();

    REQUIRE(nativeContext != nullptr);

    const std::int64_t keyCountBefore =
        context.keyValueCount();

    context.reset(
        nativeContext
        );

    REQUIRE(context.isValid());
    REQUIRE(context.context() == nativeContext);

    REQUIRE(
        context.keyValueCount() ==
        keyCountBefore
        );
}

TEST_CASE(
    "GGUF context reports missing keys",
    "[gguf][context][edge][kv][missing]"
    )
{
    JobGgufContext context;

    REQUIRE_FALSE(
        context.hasKey(
            "job.missing"
            )
        );

    REQUIRE(
        context.keyIndex(
            "job.missing"
            ) == -1
        );

    REQUIRE(
        context.keyValue(
            "job.missing"
            ) == nullptr
        );

    REQUIRE(
        context.removeKey(
            "job.missing"
            ) == -1
        );
}

TEST_CASE(
    "GGUF context rejects invalid key indexes",
    "[gguf][context][edge][kv][index]"
    )
{
    JobGgufContext context;

    context.setKeyValue(
        JobGgufKv{
            "job.value",
            std::uint32_t{42}
        }
        );

    REQUIRE_THROWS_AS(
        context.key(-1),
        std::out_of_range
        );

    REQUIRE_THROWS_AS(
        context.key(1),
        std::out_of_range
        );

    REQUIRE_THROWS_AS(
        context.valueType(-1),
        std::out_of_range
        );

    REQUIRE_THROWS_AS(
        context.ggufValueType(1),
        std::out_of_range
        );

    REQUIRE_THROWS_AS(
        context.arrayElementType(-1),
        std::out_of_range
        );

    REQUIRE_THROWS_AS(
        context.keyValue(1),
        std::out_of_range
        );
}

TEST_CASE(
    "GGUF context reports missing tensors",
    "[gguf][context][edge][tensor][missing]"
    )
{
    JobGgufContext context;

    REQUIRE_FALSE(
        context.hasTensor(
            "job.missing.tensor"
            )
        );

    REQUIRE(
        context.tensorIndex(
            "job.missing.tensor"
            ) == -1
        );
}

TEST_CASE(
    "GGUF context rejects invalid tensor indexes",
    "[gguf][context][edge][tensor][index]"
    )
{
    JobGgufContext context;

    REQUIRE_THROWS_AS(
        context.tensorName(-1),
        std::out_of_range
        );

    REQUIRE_THROWS_AS(
        context.tensorType(0),
        std::out_of_range
        );

    REQUIRE_THROWS_AS(
        context.ggmlTensorType(0),
        std::out_of_range
        );

    REQUIRE_THROWS_AS(
        context.tensorSize(0),
        std::out_of_range
        );

    REQUIRE_THROWS_AS(
        context.tensorOffset(0),
        std::out_of_range
        );
}

TEST_CASE(
    "GGUF context rejects an unnamed tensor",
    "[gguf][context][edge][tensor][name]"
    )
{
    constexpr std::int64_t elementCount = 4;

    constexpr std::size_t payloadBytes =
        static_cast<std::size_t>(
            elementCount
            ) * sizeof(float);

    auto tensorContext =
        createAllocatedHostContext(
            1,
            payloadBytes
            );

    REQUIRE(tensorContext != nullptr);

    auto tensor =
        tensorContext->newTensor1d(
            JobGgmlType::F32,
            elementCount
            );

    REQUIRE(tensor != nullptr);
    REQUIRE_FALSE(tensor->hasName());

    JobGgufContext context;

    REQUIRE_THROWS_AS(
        context.addTensor(
            *tensor
            ),
        std::invalid_argument
        );
}

TEST_CASE(
    "GGUF context rejects duplicate tensor names",
    "[gguf][context][edge][tensor][duplicate]"
    )
{
    constexpr std::int64_t elementCount = 4;

    constexpr std::size_t payloadBytes =
        static_cast<std::size_t>(
            elementCount
            ) * sizeof(float);

    auto tensorContext =
        createAllocatedHostContext(
            2,
            payloadBytes * 2
            );

    REQUIRE(tensorContext != nullptr);

    auto first =
        tensorContext->newTensor1d(
            JobGgmlType::F32,
            elementCount
            );

    auto second =
        tensorContext->newTensor1d(
            JobGgmlType::F32,
            elementCount
            );

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    first->setName(
        "job.duplicate.tensor"
        );

    second->setName(
        "job.duplicate.tensor"
        );

    JobGgufContext context;

    context.addTensor(
        *first
        );

    REQUIRE_THROWS_AS(
        context.addTensor(
            *second
            ),
        std::invalid_argument
        );

    REQUIRE(context.tensorCount() == 1);
}

TEST_CASE(
    "GGUF context rejects tensor mutation for missing names",
    "[gguf][context][edge][tensor][mutation]"
    )
{
    JobGgufContext context;

    REQUIRE_THROWS_AS(
        context.setTensorType(
            "job.missing.tensor",
            JobGgmlType::F16
            ),
        std::invalid_argument
        );

    const std::vector<std::byte> payload{
        std::byte{0}
    };

    REQUIRE_THROWS_AS(
        context.setTensorData(
            "job.missing.tensor",
            payload
            ),
        std::invalid_argument
        );
}

TEST_CASE(
    "GGUF context rejects undersized tensor payload vectors",
    "[gguf][context][edge][tensor][data]"
    )
{
    constexpr std::int64_t elementCount = 8;

    constexpr std::size_t payloadBytes =
        static_cast<std::size_t>(
            elementCount
            ) * sizeof(float);

    auto tensorContext =
        createAllocatedHostContext(
            1,
            payloadBytes
            );

    REQUIRE(tensorContext != nullptr);

    auto tensor =
        tensorContext->newTensor1d(
            JobGgmlType::F32,
            elementCount
            );

    REQUIRE(tensor != nullptr);

    tensor->setName(
        "job.payload.tensor"
        );

    JobGgufContext context;

    context.addTensor(
        *tensor
        );

    const std::vector<std::byte> undersizedPayload(
        payloadBytes - 1
        );

    REQUIRE_THROWS_AS(
        context.setTensorData(tensor->name(), undersizedPayload),
        std::invalid_argument
        );
}

TEST_CASE("Invalid GGUF context inspection returns neutral values", "[gguf][context][edge][invalid]")
{
    JobGgufContext context;

    context.reset();

    REQUIRE_FALSE(context.isValid());

    REQUIRE(context.version() == 0);
    REQUIRE(context.alignment() == 0);
    REQUIRE(context.dataOffset() == 0);
    REQUIRE(context.metadataSize() == 0);

    REQUIRE(context.keyValueCount() == 0);
    REQUIRE(context.tensorCount() == 0);

    REQUIRE_FALSE( context.hasKey( "job.key" ) );

    REQUIRE_FALSE( context.hasTensor( "job.tensor" ) );

    REQUIRE( context.keyIndex( "job.key" ) == -1 );

    REQUIRE( context.tensorIndex( "job.tensor" ) == -1 );

    REQUIRE( context.keyValues().empty() );
}

// Block three: benchmarks / stress
#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("GGUF context key insertion performance", "[gguf][context][benchmark][kv][insert]")
{
    BENCHMARK("insert 1024 GGUF key values") {
        JobGgufContext context;
        for (std::size_t index = 0; index < 1024; ++index) {
            context.setKeyValue(JobGgufKv{
                "job.context.key." + std::to_string(index),
                static_cast<std::uint64_t>( index )
            });
        }

        return context.keyValueCount();
    };
}

TEST_CASE("GGUF context key lookup performance", "[gguf][context][benchmark][kv][lookup]")
{
    JobGgufContext context;
    constexpr std::size_t keyCount = 1024;
    for (std::size_t index = 0;
         index < keyCount;
         ++index) {
        context.setKeyValue(JobGgufKv{
            "job.context.key." + std::to_string(index),
            static_cast<std::uint64_t>(index)
        });
    }

    REQUIRE(context.keyValueCount() == static_cast<std::int64_t>(keyCount));

    BENCHMARK("find one GGUF context key"){
        return context.keyIndex("job.context.key.777");
    };
}

TEST_CASE("GGUF context typed value reconstruction performance", "[gguf][context][benchmark][kv][value]")
{
    JobGgufContext context;

    context.setKeyValue(JobGgufKv{
        "job.dimensions",
        std::vector<std::uint32_t>{
            64,
            128,
            256,
            512
        }
    });

    BENCHMARK("reconstruct one context key value"){
        return context.keyValue("job.dimensions");
    };
}

TEST_CASE("GGUF context key enumeration performance", "[gguf][context][benchmark][kv][enumeration]")
{
    JobGgufContext context;

    constexpr std::size_t keyCount = 1024;

    for (std::size_t index = 0; index < keyCount; ++index) {
        context.setKeyValue( JobGgufKv{
            "job.context.key." + std::to_string(index),
            static_cast<std::uint32_t>(index)
        });
    }

    BENCHMARK("reconstruct every GGUF context key value"){
        return context.keyValues();
    };
}

TEST_CASE("GGUF context metadata serialization performance", "[gguf][context][benchmark][metadata]")
{
    JobGgufContext context;

    constexpr std::size_t keyCount = 1024;

    for (std::size_t index = 0; index < keyCount; ++index) {
        context.setKeyValue(JobGgufKv{
            "job.context.metadata." + std::to_string(index),
            static_cast<std::uint64_t>( index )
        });
    }

    REQUIRE(context.metadataSize() > 0);
    BENCHMARK( "serialize GGUF context metadata" ) {
        return context.metadata();
    };
}

TEST_CASE( "GGUF context reset performance", "[gguf][context][benchmark][reset]" )
{
    BENCHMARK( "create and reset one GGUF context" )
    {
        JobGgufContext context;

        context.setKeyValue( JobGgufKv{ "job.value", std::uint32_t{42} } );
        gguf_context *replacement = gguf_init_empty();
        context.reset(replacement);
        return context.isValid();
    };
}

#endif