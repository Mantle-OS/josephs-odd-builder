#include <cstddef>
#include <cstdint>
#include <string>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <ggml.h>

#include <job_ggml_context.h>
#include <job_ggml_enums.h>
#include <job_ggml_init_params.h>
#include <job_ggml_tensor.h>
#include <job_gguf_tensor_info.h>

#include "test_ggml_utils.h"

using namespace job::ggml;

// Block one: usage / examples
TEST_CASE("GGUF tensor info copies tensor metadata", "[gguf][tensor_info][usage][metadata]")
{
    auto context = JobGgmlContext::createUniqMetadata(1);

    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());
    REQUIRE(context->noAlloc());

    auto tensor = JobGgmlTensor::createUniqNamedTensor2d( *context,
                                                         "blk.0.attn_q.weight",
                                                         JobGgmlType::F32,
                                                         8,
                                                         4);

    REQUIRE(tensor != nullptr);
    REQUIRE(tensor->isValid());

    JobGgufTensorInfo info{ *tensor, 128 };
    REQUIRE(info.isValid());

    REQUIRE(info.name() == "blk.0.attn_q.weight");
    REQUIRE(info.type() == JobGgmlType::F32);
    REQUIRE(info.ggmlType() == GGML_TYPE_F32);
    REQUIRE(info.rank() == 2);

    REQUIRE(info.extent(0) == 8);
    REQUIRE(info.extent(1) == 4);
    REQUIRE(info.extent(2) == 1);
    REQUIRE(info.extent(3) == 1);

    REQUIRE(info.stride(0) == sizeof(float));
    REQUIRE(info.stride(1) == 8 * sizeof(float));
    REQUIRE(info.stride(2) == 8 * 4 * sizeof(float));
    REQUIRE(info.stride(3) == 8 * 4 * sizeof(float));

    REQUIRE(info.elementCount() == 32);
    REQUIRE(info.byteCount() == 32 * sizeof(float));

    REQUIRE_FALSE(info.isQuantized());

    REQUIRE(info.offset() == 128);
    REQUIRE(info.isAligned(32));
    REQUIRE(info.isAligned(64));
    REQUIRE_FALSE(info.isAligned(256));
}

TEST_CASE("GGUF tensor info owns an independent metadata copy", "[gguf][tensor_info][usage][copy]")
{
    auto context = JobGgmlContext::createUniqMetadata(1);
    auto tensor = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                         "original.weight",
                                                         JobGgmlType::F32,
                                                         16,
                                                         2);

    REQUIRE(tensor != nullptr);

    JobGgufTensorInfo info{ *tensor, 64 };
    REQUIRE(info.isValid());
    REQUIRE(info.name() == "original.weight");
    REQUIRE(info.type() == JobGgmlType::F32);
    REQUIRE(info.extent(0) == 16);
    REQUIRE(info.extent(1) == 2);

    /*
     * JobGgufTensorInfo copies the native ggml_tensor metadata. Later
     * mutations to the source tensor must not alter the GGUF record.
     */
    tensor->setName("source.changed");

    REQUIRE(tensor->name() == "source.changed");
    REQUIRE(info.name() == "original.weight");

    info.setName("gguf.changed");

    REQUIRE(info.name() == "gguf.changed");
    REQUIRE(tensor->name() == "source.changed");
}

TEST_CASE("GGUF tensor info name may be changed independently", "[gguf][tensor_info][usage][name]")
{
    auto context = JobGgmlContext::createUniqMetadata(1);
    auto tensor = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                         "before",
                                                         JobGgmlType::F32,
                                                         4,
                                                         4);

    REQUIRE(tensor != nullptr);

    JobGgufTensorInfo info{ *tensor, 0 };
    REQUIRE(info.name() == "before");

    info.setName("after");
    REQUIRE(info.name() == "after");
    REQUIRE(info.isValid());

    // The source tensor is not renamed because JobGgufTensorInfo stores its own native metadata value.
    REQUIRE(tensor->name() == "before");
}

TEST_CASE("GGUF tensor info offset and alignment may be updated", "[gguf][tensor_info][usage][alignment]")
{
    auto context = JobGgmlContext::createUniqMetadata(1);
    auto tensor = JobGgmlTensor::createUniqNamedTensor2d(*context, "aligned.weight");

    REQUIRE(tensor != nullptr);

    JobGgufTensorInfo info{ *tensor, 32 };
    REQUIRE(info.offset() == 32);
    REQUIRE(info.isAligned(32));
    REQUIRE_FALSE(info.isAligned(64));

    info.setOffset(256);
    REQUIRE(info.offset() == 256);

    REQUIRE(info.isAligned(1));
    REQUIRE(info.isAligned(2));
    REQUIRE(info.isAligned(4));
    REQUIRE(info.isAligned(8));
    REQUIRE(info.isAligned(16));
    REQUIRE(info.isAligned(32));
    REQUIRE(info.isAligned(64));
    REQUIRE(info.isAligned(128));
    REQUIRE(info.isAligned(256));
    REQUIRE_FALSE(info.isAligned(512));
}

TEST_CASE("GGUF tensor info calculates aligned payload sizes", "[gguf][tensor_info][usage][padding]" )
{
    auto context = JobGgmlContext::createUniqMetadata(1);
    auto tensor = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                         "padding.weight",
                                                         JobGgmlType::F32,
                                                         3,
                                                         3);

    REQUIRE(tensor != nullptr);

    JobGgufTensorInfo info{ *tensor, 0 };
    REQUIRE(info.byteCount() == 36);

    REQUIRE(info.paddedByteCount(1) == 36);
    REQUIRE(info.paddedByteCount(2) == 36);
    REQUIRE(info.paddedByteCount(4) == 36);
    REQUIRE(info.paddedByteCount(8) == 40);
    REQUIRE(info.paddedByteCount(16) == 48);
    REQUIRE(info.paddedByteCount(32) == 64);
    REQUIRE(info.paddedByteCount(64) == 64);
}

TEST_CASE("GGUF tensor info type mutation recalculates tensor strides", "[gguf][tensor_info][usage][type]")
{
    auto context = JobGgmlContext::createUniqMetadata(1);
    auto tensor = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                         "typed.weight",
                                                         JobGgmlType::F32,
                                                         8,
                                                         4);
    REQUIRE(tensor != nullptr);

    JobGgufTensorInfo info{ *tensor, 0 };

    REQUIRE(info.type() == JobGgmlType::F32);
    REQUIRE(info.byteCount() == 128);

    info.setType(JobGgmlType::I32);

    REQUIRE(info.type() == JobGgmlType::I32);
    REQUIRE(info.ggmlType() == GGML_TYPE_I32);

    REQUIRE(info.stride(0) == sizeof(std::int32_t));
    REQUIRE(info.stride(1) == 8 * sizeof(std::int32_t));
    REQUIRE(info.stride(2) == 8 * 4 * sizeof(std::int32_t));

    REQUIRE(info.elementCount() == 32);
    REQUIRE(info.byteCount() == 32 * sizeof(std::int32_t));

    REQUIRE_FALSE(info.isQuantized());

    info.setGgmlType(GGML_TYPE_F16);

    REQUIRE(info.ggmlType() == GGML_TYPE_F16);
    REQUIRE(info.type() == JobGgmlType::F16);
    REQUIRE(info.stride(0) == ggml_type_size(GGML_TYPE_F16));
    REQUIRE(info.stride(1) == 8 * ggml_type_size(GGML_TYPE_F16));
    REQUIRE(info.byteCount() == 32 * ggml_type_size(GGML_TYPE_F16));
}

TEST_CASE("GGUF tensor info supports quantized tensor metadata", "[gguf][tensor_info][usage][quantized]")
{
    constexpr std::int64_t RowExtent = 32;
    constexpr std::int64_t RowCount  = 4;

    auto context = JobGgmlContext::createUniqMetadata(1);
    auto tensor = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                         "quantized.weight",
                                                         JobGgmlType::F32,
                                                         RowExtent,
                                                         RowCount);
    REQUIRE(tensor != nullptr);

    JobGgufTensorInfo info{ *tensor, 0 };

    const std::int64_t blockSize = ggml_blck_size(GGML_TYPE_Q4_0);
    const std::size_t typeSize = ggml_type_size(GGML_TYPE_Q4_0);

    REQUIRE(blockSize > 0);
    REQUIRE(typeSize > 0);
    REQUIRE(RowExtent % blockSize == 0);

    info.setGgmlType(GGML_TYPE_Q4_0);

    REQUIRE(info.isValid());
    REQUIRE(info.isQuantized());

    REQUIRE(info.ggmlType() == GGML_TYPE_Q4_0);
    REQUIRE(info.type() == JobGgmlType::Q4_0);

    REQUIRE(info.rank() == 2);
    REQUIRE(info.extent(0) == RowExtent);
    REQUIRE(info.extent(1) == RowCount);

    REQUIRE(info.stride(0) == typeSize);
    REQUIRE(info.stride(1) == typeSize * static_cast<std::size_t>( RowExtent / blockSize));
    REQUIRE(info.stride(2) == info.stride(1) * static_cast<std::size_t>(RowCount));

    REQUIRE(info.byteCount() == info.stride(1) * static_cast<std::size_t>(RowCount));
}

TEST_CASE( "GGUF tensor info may replace its copied tensor metadata", "[gguf][tensor_info][usage][set_tensor]")
{
    auto context = JobGgmlContext::createUniqMetadata(2);

    auto first = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                        "first.weight",
                                                        JobGgmlType::F32,
                                                        8,
                                                        4);

    auto second = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                         "second.weight",
                                                         JobGgmlType::I32,
                                                         6,
                                                         5);

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    JobGgufTensorInfo info{ *first, 128 };
    REQUIRE(info.name() == "first.weight");
    REQUIRE(info.type() == JobGgmlType::F32);
    REQUIRE(info.extent(0) == 8);
    REQUIRE(info.extent(1) == 4);
    REQUIRE(info.offset() == 128);

    info.setTensor(*second);

    REQUIRE(info.isValid());
    REQUIRE(info.name() == "second.weight");
    REQUIRE(info.type() == JobGgmlType::I32);
    REQUIRE(info.extent(0) == 6);
    REQUIRE(info.extent(1) == 5);

    /*
     * Replacing tensor metadata does not alter the independently stored
     * GGUF data offset.
     */
    REQUIRE(info.offset() == 128);
}

// Block two: edge cases / invariants
TEST_CASE( "GGUF tensor info requires a named source tensor", "[gguf][tensor_info][edge][validity]")
{
    auto context = JobGgmlContext::createUniqMetadata(1);
    auto tensor = context->newTensor1d(JobGgmlType::F32, 8);

    REQUIRE(tensor != nullptr);
    REQUIRE(tensor->isValid());
    REQUIRE_FALSE(tensor->hasName());

    JobGgufTensorInfo info{ *tensor, 0 };

    /*
     * Construction is allowed because the source JobGgmlTensor itself is
     * valid, but a GGUF tensor record requires a non-empty name.
     */
    REQUIRE_FALSE(info.isValid());

    REQUIRE(info.name().empty());
    REQUIRE(info.rank() == 0);
    REQUIRE(info.elementCount() == 0);
    REQUIRE(info.byteCount() == 0);
    REQUIRE_FALSE(info.isQuantized());

    info.setName("now.valid");

    REQUIRE(info.isValid());
    REQUIRE(info.name() == "now.valid");
    REQUIRE(info.rank() == 1);
    REQUIRE(info.elementCount() == 8);
}

TEST_CASE("GGUF tensor info rejects invalid names", "[gguf][tensor_info][edge][name]")
{
    auto context = JobGgmlContext::createUniqMetadata(1);
    auto tensor = JobGgmlTensor::createUniqNamedTensor2d(*context, "valid.weight");
    REQUIRE(tensor != nullptr);

    JobGgufTensorInfo info{ *tensor, 0 };
    REQUIRE_THROWS_AS(info.setName(""), std::invalid_argument);

    const std::string maximumValidName(GGML_MAX_NAME - 1, 'a');  // eh canda ....
    REQUIRE_NOTHROW( info.setName(maximumValidName) );
    REQUIRE(info.name() == maximumValidName);

    const std::string oversizedName(GGML_MAX_NAME, 'b');  // cannot think of any funny for b is for buffer overflow not funny at all
    REQUIRE_THROWS_AS(info.setName(oversizedName), std::invalid_argument);
    REQUIRE(info.name() == maximumValidName);
}

TEST_CASE("GGUF tensor info rejects invalid GGML types", "[gguf][tensor_info][edge][type]")
{
    auto context = JobGgmlContext::createUniqMetadata(1);
    auto tensor = JobGgmlTensor::createUniqNamedTensor2d(*context, "type.weight");
    REQUIRE(tensor != nullptr);

    JobGgufTensorInfo info{ *tensor, 0 };
    REQUIRE_THROWS_AS(info.setGgmlType(static_cast<enum ggml_type>(-1)), std::invalid_argument);
    REQUIRE_THROWS_AS(info.setGgmlType( static_cast<enum ggml_type>(GGML_TYPE_COUNT)), std::invalid_argument);

    REQUIRE(info.ggmlType() == GGML_TYPE_F32);
}

TEST_CASE("GGUF tensor info enforces quantized block alignment", "[gguf][tensor_info][edge][quantized][block]")
{
    auto context = JobGgmlContext::createUniqMetadata(1);
    auto tensor = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                         "unaligned.weight",
                                                         JobGgmlType::F32,
                                                         31,
                                                         4);

    REQUIRE(tensor != nullptr);

    JobGgufTensorInfo info{ *tensor, 0 };
    REQUIRE(ggml_blck_size(GGML_TYPE_Q4_0) > 1);
    REQUIRE_THROWS_AS(info.setGgmlType(GGML_TYPE_Q4_0), std::invalid_argument);
    /*
     * Failed mutation must preserve the previous valid type and layout.
     */
    REQUIRE(info.ggmlType() == GGML_TYPE_F32);
    REQUIRE(info.type() == JobGgmlType::F32);
    REQUIRE(info.stride(0) == sizeof(float));
}

TEST_CASE("GGUF tensor info rejects invalid dimensions safely", "[gguf][tensor_info][edge][dimension]")
{
    auto context = JobGgmlContext::createUniqMetadata(1);
    auto tensor = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                         "dimensions.weight",
                                                         JobGgmlType::F32,
                                                         8,
                                                         4);
    REQUIRE(tensor != nullptr);

    JobGgufTensorInfo info{ *tensor, 0 };
    REQUIRE(info.extent(GGML_MAX_DIMS) == 0);
    REQUIRE(info.stride(GGML_MAX_DIMS) == 0);

    REQUIRE(info.extent(GGML_MAX_DIMS + 100) == 0);
    REQUIRE(info.stride(GGML_MAX_DIMS + 100) == 0 );
}

TEST_CASE("GGUF tensor info rejects invalid alignments", "[gguf][tensor_info][edge][alignment]")
{
    auto context = JobGgmlContext::createUniqMetadata(1);
    auto tensor = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                         "alignment.weight",
                                                         JobGgmlType::F32,
                                                         8,
                                                         4);
    REQUIRE(tensor != nullptr);

    JobGgufTensorInfo info{ *tensor, 96 };
    REQUIRE_FALSE(info.isAligned(0));
    REQUIRE_FALSE(info.isAligned(3));
    REQUIRE_FALSE(info.isAligned(6));
    REQUIRE_FALSE(info.isAligned(12));

    REQUIRE(info.isAligned(1));
    REQUIRE(info.isAligned(2));
    REQUIRE(info.isAligned(4));
    REQUIRE(info.isAligned(8));
    REQUIRE(info.isAligned(16));
    REQUIRE(info.isAligned(32));

    REQUIRE(info.paddedByteCount(0) == 0);
    REQUIRE(info.paddedByteCount(3) == 0);
    REQUIRE(info.paddedByteCount(6) == 0);
    REQUIRE(info.paddedByteCount(12) == 0);
}

TEST_CASE("GGUF tensor info reset clears all record state", "[gguf][tensor_info][edge][reset]")
{
    auto context = JobGgmlContext::createUniqMetadata(1);
    auto tensor = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                         "reset.weight",
                                                         JobGgmlType::F32,
                                                         8,
                                                         4);

    REQUIRE(tensor != nullptr);

    JobGgufTensorInfo info{ *tensor, 256 };
    REQUIRE(info.isValid());
    REQUIRE(info.name() == "reset.weight");
    REQUIRE(info.offset() == 256);
    REQUIRE(info.elementCount() == 32);
    REQUIRE(info.byteCount() == 128);

    info.reset();
    REQUIRE_FALSE(info.isValid());
    REQUIRE(info.name().empty());
    REQUIRE(info.offset() == 0);
    REQUIRE(info.rank() == 0);
    REQUIRE(info.elementCount() == 0);
    REQUIRE(info.byteCount() == 0);
    REQUIRE(info.extent(0) == 0);
    REQUIRE(info.stride(0) == 0);
    REQUIRE_FALSE(info.isQuantized());
    REQUIRE(info.paddedByteCount(32) == 0);
}

TEST_CASE("GGUF tensor info may be reused after reset", "[gguf][tensor_info][edge][reset][reuse]")
{
    auto context = JobGgmlContext::createUniqMetadata(2);
    auto first = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                        "first.weight",
                                                        JobGgmlType::F32,
                                                        8,
                                                        4);

    auto second = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                         "second.weight",
                                                         JobGgmlType::I32,
                                                         4,
                                                         3);

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    JobGgufTensorInfo info{ *first, 128 };

    REQUIRE(info.isValid());
    info.reset();
    REQUIRE_FALSE(info.isValid());

    info.setTensor(*second);
    info.setOffset(512);
    REQUIRE(info.isValid());
    REQUIRE(info.name()             == "second.weight");
    REQUIRE(info.type()             == JobGgmlType::I32);
    REQUIRE(info.rank()             == 2);
    REQUIRE(info.elementCount()     == 12);
    REQUIRE(info.offset()           == 512);
    REQUIRE(info.isAligned(512));
}

// Block three: benchmarks / stress
#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("GGUF tensor info construction performance", "[gguf][tensor_info][benchmark][construction]")
{
    auto context = JobGgmlContext::createUniqMetadata(1);
    auto tensor = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                         "benchmark.weight",
                                                         JobGgmlType::F32,
                                                         4096,
                                                         4096);
    REQUIRE(tensor != nullptr);
    BENCHMARK("copy one GGML tensor metadata record") {
        return JobGgufTensorInfo{ *tensor, 4096 };
    };
}

TEST_CASE("GGUF tensor info metadata inspection performance", "[gguf][tensor_info][benchmark][inspection]")
{
    auto context = JobGgmlContext::createUniqMetadata(1);
    auto tensor = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                         "benchmark.inspect.weight",
                                                         JobGgmlType::F32,
                                                         4096,
                                                         4096);

    REQUIRE(tensor != nullptr);

    JobGgufTensorInfo info{ *tensor, 4096 };
    REQUIRE(info.isValid());
    BENCHMARK("inspect one GGUF tensor metadata record") {
        return static_cast<std::uint64_t>(info.elementCount())  +
               static_cast<std::uint64_t>(info.byteCount())     +
               static_cast<std::uint64_t>(info.extent(0))       +
               static_cast<std::uint64_t>(info.extent(1))       +
               static_cast<std::uint64_t>(info.stride(0))       +
               static_cast<std::uint64_t>(info.stride(1))       +
               info.offset();
    };
}

TEST_CASE("GGUF tensor info type conversion performance", "[gguf][tensor_info][benchmark][type]")
{
    auto context = JobGgmlContext::createUniqMetadata(1);
    auto tensor = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                         "benchmark.type.weight",
                                                         JobGgmlType::F32,
                                                         4096,
                                                         4096);
    REQUIRE(tensor != nullptr);
    BENCHMARK("copy metadata and apply F16 type"){
        JobGgufTensorInfo info{ *tensor, 0 };
        info.setGgmlType(GGML_TYPE_F16);
        return info.byteCount();
    };
}

TEST_CASE("GGUF tensor info repeated metadata replacement", "[gguf][tensor_info][stress][replacement]")
{
    constexpr std::size_t IterationCount = 10000;
    auto context = JobGgmlContext::createUniqMetadata(2);

    auto first = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                        "first.weight",
                                                        JobGgmlType::F32,
                                                        64,
                                                        64);

    auto second = JobGgmlTensor::createUniqNamedTensor2d(*context,
                                                         "second.weight",
                                                         JobGgmlType::I32,
                                                         32,
                                                         32);

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    JobGgufTensorInfo info{ *first, 0 };
    for (std::size_t index = 0; index < IterationCount; ++index) {
        if ((index & 1U) == 0)
            info.setTensor(*second);
        else
            info.setTensor(*first);

        info.setOffset(static_cast<std::uint64_t>(index * 32));
        REQUIRE(info.isValid());
        REQUIRE(info.isAligned(32));
    }

    REQUIRE(info.offset() == static_cast<std::uint64_t>((IterationCount - 1) * 32));
    REQUIRE(info.name() == "first.weight");
    REQUIRE(info.type() == JobGgmlType::F32);
}

#endif