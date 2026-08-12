#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>

#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_ggml_context.h>
#include <job_ggml_enums.h>
#include <job_ggml_tensor.h>

#include <job_gguf.h>
#include <job_gguf_context.h>
#include <job_gguf_init_params.h>
#include <job_gguf_kv.h>
#include <job_gguf_reader.h>
#include <job_gguf_tensor_info.h>
#include <job_gguf_type_traits.h>
#include <job_gguf_writer.h>

#include "test_ggml_utils.h"
// optional use the
TEST_CASE("JobGgml exposes an owned GGUF facade", "[gguf][job_ggml][ownership]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGguf *gguf = g_jobGgml->gguf();

    REQUIRE(gguf != nullptr);
    REQUIRE(gguf->isValid());
    REQUIRE(gguf->context() != nullptr);
    REQUIRE(gguf->reader() != nullptr);
    REQUIRE(gguf->writer() != nullptr);
}
// Block one: usage / examples
TEST_CASE("JobGguf constructs an empty writable document", "[gguf][usage][construction]")
{
    JobGgmlContext::UPtr ggmlContext;

    JobGguf gguf{&ggmlContext};

    REQUIRE(gguf.isValid());

    REQUIRE(gguf.initParams() != nullptr);
    REQUIRE(gguf.context() != nullptr);
    REQUIRE(gguf.reader() != nullptr);
    REQUIRE(gguf.writer() != nullptr);

    REQUIRE(gguf.context()->isValid());
    REQUIRE(gguf.reader()->isValid());
    REQUIRE(gguf.writer()->isValid());

    REQUIRE(gguf.contextOutput() == &ggmlContext);

    REQUIRE_FALSE(gguf.hasContent());

    REQUIRE(gguf.keyValueCount() == 0);
    REQUIRE(gguf.tensorCount() == 0);

    REQUIRE(ggmlContext == nullptr);
    REQUIRE_FALSE(gguf.hasError());
}

TEST_CASE("JobGguf stores and retrieves typed scalar metadata", "[gguf][usage][kv][scalar]")
{
    JobGguf gguf;

    gguf.setKeyValue(JobGgufKv{ "job.u32",      std::uint32_t{42} });
    gguf.setKeyValue(JobGgufKv{ "job.i64",      std::int64_t{-123456} });
    gguf.setKeyValue(JobGgufKv{ "job.f32",      3.5f });
    gguf.setKeyValue(JobGgufKv{ "job.f64",      9.25 });
    gguf.setKeyValue(JobGgufKv{ "job.bool",     true });
    gguf.setKeyValue(JobGgufKv{ "job.string",   std::string{"glasses"} });
    REQUIRE(gguf.keyValueCount() == 6);

    {
        auto value = gguf.keyValue("job.u32");

        REQUIRE(value != nullptr);
        REQUIRE(value->isScalar());
        REQUIRE_FALSE(value->isArray());
        REQUIRE(value->type() == JobGgufType::UInt32);
        REQUIRE(value->value<std::uint32_t>() == std::uint32_t{42});
    }

    {
        auto value = gguf.keyValue("job.i64");
        REQUIRE(value != nullptr);
        REQUIRE(value->isScalar());
        REQUIRE(value->type() == JobGgufType::Int64);
        REQUIRE(value->value<std::int64_t>() == std::int64_t{-123456});
    }

    {
        auto value =gguf.keyValue("job.f32");
        REQUIRE(value != nullptr);
        REQUIRE(value->isScalar());
        REQUIRE(value->type() == JobGgufType::Float32);
        REQUIRE(value->value<float>() == 3.5f);
    }

    {
        auto value = gguf.keyValue("job.f64");
        REQUIRE(value != nullptr);
        REQUIRE(value->isScalar());
        REQUIRE(value->type() == JobGgufType::Float64);
        REQUIRE(value->value<double>() == 9.25);
    }

    {
        auto value = gguf.keyValue("job.bool");
        REQUIRE(value != nullptr);
        REQUIRE(value->isScalar());
        REQUIRE(value->isBoolean());
        REQUIRE(value->value<bool>());
    }

    {
        auto value = gguf.keyValue("job.string");
        REQUIRE(value != nullptr);
        REQUIRE(value->isScalar());
        REQUIRE(value->isString());
        REQUIRE(value->value<std::string>() == "glasses");
    }
}

TEST_CASE("JobGguf stores and retrieves typed array metadata", "[gguf][usage][kv][array]")
{
    JobGguf gguf;
    const std::vector<std::uint32_t> dimensions { 64, 128, 256, 512 };
    const std::vector<bool> flags {
        true,
        false,
        true,
        true
    };

    const std::vector<std::string> labels { "alpha", "beta", "gamma" };
    gguf.setKeyValue(JobGgufKv{ "job.dimensions",   dimensions });
    gguf.setKeyValue(JobGgufKv{ "job.flags",        flags });
    gguf.setKeyValue(JobGgufKv{ "job.labels",       labels });
    REQUIRE(gguf.keyValueCount() == 3);

    {
        auto value = gguf.keyValue("job.dimensions");
        REQUIRE(value != nullptr);
        REQUIRE(value->isArray());
        REQUIRE_FALSE(value->isScalar());
        REQUIRE(value->serializedType() == JobGgufType::Array);
        REQUIRE(value->type() == JobGgufType::UInt32);
        REQUIRE(value->elementCount() == dimensions.size());
        REQUIRE(value->values<std::uint32_t>() == dimensions);
    }

    {
        auto value = gguf.keyValue("job.flags");
        REQUIRE(value != nullptr);
        REQUIRE(value->isArray());
        REQUIRE(value->isBoolean());
        REQUIRE(value->type() == JobGgufType::Bool);
        REQUIRE(value->values<bool>() == flags);
    }

    {
        auto value = gguf.keyValue("job.labels");
        REQUIRE(value != nullptr);
        REQUIRE(value->isArray());
        REQUIRE(value->isString());
        REQUIRE(value->type() == JobGgufType::String);
        REQUIRE(value->values<std::string>() == labels);
    }
}

TEST_CASE("JobGguf replaces an existing key value", "[gguf][usage][kv][replace]")
{
    JobGguf gguf;
    gguf.setKeyValue( JobGgufKv{ "job.value", std::uint32_t{10} });
    REQUIRE(gguf.keyValueCount() == 1);

    gguf.setKeyValue(JobGgufKv{ "job.value", std::uint32_t{20} });
    REQUIRE(gguf.keyValueCount() == 1);

    auto value = gguf.keyValue("job.value");
    REQUIRE(value != nullptr);
    REQUIRE(value->value<std::uint32_t>() == std::uint32_t{20});
}

TEST_CASE( "JobGguf removes metadata by key", "[gguf][usage][kv][remove]")
{
    JobGguf gguf;
    populateExampleMetadata(gguf);
    REQUIRE(gguf.hasKey("job.scale"));

    const std::int64_t previousIndex = gguf.removeKey("job.scale");
    REQUIRE(previousIndex >= 0);
    REQUIRE_FALSE(gguf.hasKey("job.scale"));

    REQUIRE(gguf.removeKey("job.scale") == -1);
}

TEST_CASE("JobGguf writes and reopens a metadata document", "[gguf][usage][io][roundtrip]")
{
    TransientTestFile temporaryFile{
        transientPath("job_gguf_metadata_roundtrip.gguf")
    };

    JobGguf source;
    populateExampleMetadata(source);

    REQUIRE(source.hasContent());
    REQUIRE(source.tensorCount() == 0);
    REQUIRE(source.save(temporaryFile.path()));
    REQUIRE(std::filesystem::is_regular_file(temporaryFile.path()));

    JobGguf destination;

    destination.initParams()->setNoAlloc(true);
    destination.initParams()->setCreateContext(false);

    REQUIRE(destination.open(temporaryFile.path()));
    REQUIRE(destination.keyValueCount() == source.keyValueCount());
    REQUIRE(destination.tensorCount() == 0);

    auto name = destination.keyValue("general.name");
    REQUIRE(name != nullptr);

    REQUIRE(name->value<std::string>() == "Joseph's Odd Builder GGUF Test");

    auto dimensions =destination.keyValue("job.dimensions");
    REQUIRE(dimensions != nullptr);

    REQUIRE(dimensions->values<std::uint32_t>() == std::vector<std::uint32_t>{ 64, 128, 256 });
}

TEST_CASE("JobGguf reads serialized data from memory", "[gguf][usage][io][buffer]")
{
    TransientTestFile temporaryFile{
        transientPath("job_gguf_memory_roundtrip.gguf")
    };

    JobGguf source;
    populateExampleMetadata(source);
    REQUIRE(source.save(temporaryFile.path()));
    const std::vector<std::byte> fileData = readFileBytes(temporaryFile.path());
    REQUIRE_FALSE(fileData.empty());

    JobGguf destination;
    destination.initParams()->setNoAlloc(true);
    destination.initParams()->setCreateContext(false);
    REQUIRE(destination.open( std::span<const std::byte>{
        fileData.data(),
        fileData.size()
    }));
    REQUIRE(destination.hasKey("general.architecture"));

    auto architecture = destination.keyValue("general.architecture");
    REQUIRE(architecture != nullptr);
    REQUIRE(architecture->value<std::string>() =="job-test");
}

TEST_CASE("JobGguf reads serialized data from a FILE pointer", "[gguf][usage][io][file_ptr]")
{
    TransientTestFile temporaryFile{
        transientPath("job_gguf_file_pointer_roundtrip.gguf")
    };

    JobGguf source;
    populateExampleMetadata(source);
    REQUIRE(source.save(temporaryFile.path()));

    std::FILE *file = std::fopen(temporaryFile.path().c_str(), "rb");
    REQUIRE(file != nullptr);

    JobGguf destination;
    destination.initParams()->setNoAlloc(true);
    destination.initParams()->setCreateContext(false);
    const bool opened = destination.open(file);

    std::fclose(file);

    REQUIRE(opened);
    REQUIRE(destination.keyValueCount() == source.keyValueCount());
    REQUIRE(destination.hasKey("job.context_length"));
}

TEST_CASE("JobGguf writes serialized data to a FILE pointer", "[gguf][usage][io][file_ptr][write]")
{
    TransientTestFile temporaryFile{
        transientPath("job_gguf_file_pointer_write.gguf")
    };

    JobGguf source;
    populateExampleMetadata(source);

    std::FILE *file = std::fopen(temporaryFile.path().c_str(), "wb");
    REQUIRE(file != nullptr);
    const bool saved = source.save(file, false);

    std::fclose(file);

    REQUIRE(saved);

    JobGguf destination;
    destination.initParams()->setNoAlloc(true);
    destination.initParams()->setCreateContext(false);
    REQUIRE(destination.open(temporaryFile.path()));
    REQUIRE(destination.keyValueCount() == source.keyValueCount());
}

TEST_CASE("JobGguf reads serialized data through a random access callback", "[gguf][usage][io][callback]")
{
    TransientTestFile temporaryFile{
        transientPath( "job_gguf_callback_roundtrip.gguf" )
    };

    JobGguf source;
    populateExampleMetadata(source);
    REQUIRE(source.save(temporaryFile.path()));
    const std::vector<std::byte> fileData = readFileBytes(temporaryFile.path());
    REQUIRE_FALSE(fileData.empty());

    JobGguf destination;

    destination.initParams()->setNoAlloc(true);
    destination.initParams()->setCreateContext(false);

    JobGgufReader::ReadCallback callback = [&fileData](void *output, std::uint64_t offset, std::size_t length) -> std::size_t {
        if (!output)
            return 0;

        if (offset >= fileData.size())
            return 0;

        const std::size_t sourceOffset = static_cast<std::size_t>(offset);
        const std::size_t available = fileData.size() - sourceOffset;
        const std::size_t byteCount = std::min(available, length);
        std::memcpy(output,
                    fileData.data() + sourceOffset,
                    byteCount);

        return byteCount;
    };

    REQUIRE(destination.open(std::move(callback),
                             64,
                             static_cast<std::uint64_t>(fileData.size())));

    REQUIRE(destination.keyValueCount() == source.keyValueCount());
    REQUIRE(destination.hasKey("job.labels"));
}

TEST_CASE("JobGguf exports metadata into owned and caller storage", "[gguf][usage][metadata]")
{
    JobGguf gguf;
    populateExampleMetadata(gguf);

    const std::size_t metadataSize = gguf.metadataSize();
    REQUIRE(metadataSize > 0);

    const std::vector<std::byte> ownedMetadata = gguf.metadata();
    REQUIRE(ownedMetadata.size() == metadataSize);

    std::vector<std::byte> destination(metadataSize);
    REQUIRE(gguf.writeMetadata(std::span<std::byte>{
        destination.data(),
        destination.size()
    }));
    REQUIRE(destination == ownedMetadata);
}

TEST_CASE("Serialized GGUF metadata can initialize another document", "[gguf][usage][metadata][roundtrip]")
{
    JobGguf source;

    populateExampleMetadata(source);
    const std::vector<std::byte> metadata = source.metadata();
    REQUIRE_FALSE(metadata.empty());

    TransientTestFile temporaryFile{
        transientPath("job_gguf_raw_metadata.gguf"),
        metadata
    };

    JobGguf destination;
    destination.initParams()->setNoAlloc(true);
    destination.initParams()->setCreateContext(false);
    REQUIRE(destination.open(temporaryFile.path()));
    REQUIRE(destination.keyValueCount() == source.keyValueCount());
    REQUIRE(destination.tensorCount() == 0);
}

TEST_CASE("JobGguf writes and reopens tensor metadata and payload", "[gguf][usage][tensor][roundtrip]")
{
    TransientTestFile temporaryFile{
        transientPath(
            "job_gguf_tensor_roundtrip.gguf"
            )
    };

    constexpr std::int64_t elementCount = 8;
    constexpr std::size_t payloadBytes = static_cast<std::size_t>( elementCount ) * sizeof(float);
    auto sourceContext = JobGgmlContext::createUniqHostContext(1, payloadBytes);
    REQUIRE(sourceContext != nullptr);
    REQUIRE(sourceContext->isValid());

    auto sourceTensor = sourceContext->newTensor1d(JobGgmlType::F32, elementCount);
    REQUIRE(sourceTensor != nullptr);
    REQUIRE(sourceTensor->isValid());
    REQUIRE(sourceTensor->data() != nullptr);

    sourceTensor->setName("job.test.tensor");
    REQUIRE(sourceTensor->hasName());

    for (std::int64_t index = 0; index < elementCount; ++index)
        sourceTensor->data()->setValueF32(index, static_cast<float>(index) + 0.5f);

    JobGguf source;
    source.setKeyValue( JobGgufKv{ "general.architecture", std::string{"job-test"} });
    source.addTensor(*sourceTensor);
    source.setTensorData(sourceTensor->name(), sourceTensor->dataPointer());
    REQUIRE(source.tensorCount() == 1);
    REQUIRE(source.hasTensor( "job.test.tensor"));
    REQUIRE(source.save(temporaryFile.path(), false));

    JobGgmlContext::UPtr destinationContext;
    JobGguf destination{
        &destinationContext
    };
    destination.initParams()->setNoAlloc(false);
    destination.initParams()->setCreateContext(true);
    REQUIRE(destination.open(temporaryFile.path()));
    REQUIRE(destination.tensorCount() == 1);
    REQUIRE(destination.hasTensor("job.test.tensor"));

    REQUIRE(destinationContext != nullptr);
    REQUIRE(destinationContext->isValid());

    auto destinationTensor = destinationContext->tensor("job.test.tensor");
    REQUIRE(destinationTensor != nullptr);
    REQUIRE(destinationTensor->isValid());
    REQUIRE(destinationTensor->data() != nullptr);
    REQUIRE(destinationTensor->elementCount() == elementCount);

    for (std::int64_t index = 0; index < elementCount; ++index)
        REQUIRE(destinationTensor->data()->valueF32(index) == static_cast<float>(index) + 0.5f);
}

TEST_CASE("JobGguf inspects a real GGUF model without loading tensor payloads", "[gguf][usage][integration][external]")
{
    const std::filesystem::path filePath{
        JOB_TEST_GGUF_FILE
    };
    REQUIRE_FALSE(filePath.empty());
    REQUIRE(std::filesystem::is_regular_file(filePath));

    JobGgmlContext::UPtr ggmlContext;
    JobGguf gguf{ &ggmlContext };

    gguf.initParams()->setNoAlloc(true);
    gguf.initParams()->setCreateContext(false);

    REQUIRE(gguf.open(filePath));
    REQUIRE(gguf.isValid());
    REQUIRE(gguf.hasContent());
    REQUIRE(gguf.version() > 0);
    REQUIRE(gguf.alignment() > 0);
    REQUIRE(gguf.dataOffset() > 0);
    REQUIRE(gguf.keyValueCount() > 0);
    REQUIRE(gguf.tensorCount() > 0);
    REQUIRE(ggmlContext == nullptr);
    REQUIRE(gguf.hasKey("general.architecture"));

    auto architecture = gguf.keyValue( "general.architecture");

    REQUIRE(architecture != nullptr);
    REQUIRE(architecture->isString());

    WARN("REAL MODEL:  architecture = "         << architecture->value<std::string>());
    WARN("REAL MODEL:  version = "              << gguf.version());
    WARN("REAL MODEL:  alignment = "            << gguf.alignment());
    WARN("REAL MODEL:  key/value count = "      << gguf.keyValueCount());
    WARN("REAL MODEL:  GGUF tensor count = "    << gguf.tensorCount());
}


// Block two: edge cases / contracts
TEST_CASE("GGUF enum helpers describe every supported value type", "[gguf][edge][type]")
{
    REQUIRE(static_cast<std::size_t>(JobGgufType::Count) == static_cast<std::size_t>(GGUF_TYPE_COUNT));

    REQUIRE(ggufTypeSize(JobGgufType::UInt8 )       == sizeof(std::uint8_t));
    REQUIRE(ggufTypeSize(JobGgufType::Int8 )        == sizeof(std::int8_t));
    REQUIRE(ggufTypeSize(JobGgufType::UInt16)       == sizeof(std::uint16_t));
    REQUIRE(ggufTypeSize(JobGgufType::Int16)        == sizeof(std::int16_t));
    REQUIRE(ggufTypeSize(JobGgufType::UInt32)       == sizeof(std::uint32_t));
    REQUIRE(ggufTypeSize(JobGgufType::Int32)        == sizeof(std::int32_t));
    REQUIRE(ggufTypeSize(JobGgufType::Float32)      == sizeof(float));
    REQUIRE(ggufTypeSize(JobGgufType::Bool)         == sizeof(std::int8_t));
    REQUIRE(ggufTypeSize(JobGgufType::String)       == 0);
    REQUIRE(ggufTypeSize(JobGgufType::Array)        == 0);
    REQUIRE(ggufTypeSize(JobGgufType::UInt64)       == sizeof(std::uint64_t));
    REQUIRE(ggufTypeSize(JobGgufType::Int64)        == sizeof(std::int64_t) );
    REQUIRE(ggufTypeSize(JobGgufType::Float64)      == sizeof(double) );
    REQUIRE(ggufTypeName(JobGgufType::UInt32)       == "u32");
    REQUIRE(ggufTypeName(JobGgufType::String)       == "str");
    REQUIRE(ggufTypeName(JobGgufType::Array)        == "arr");
}

TEST_CASE("GGUF enum helpers reject invalid type values", "[gguf][edge][type][invalid]")
{
    const auto invalidNative =static_cast<enum gguf_type>(GGUF_TYPE_COUNT);
    const auto invalidWrapped = static_cast<JobGgufType>(GGUF_TYPE_COUNT);
    REQUIRE_FALSE(isValidGgufType(invalidNative));
    REQUIRE_FALSE(isValidGgufType(invalidWrapped));

    REQUIRE(ggufTypeSize(invalidNative)  == 0);
    REQUIRE(ggufTypeSize(invalidWrapped) == 0);

    REQUIRE(ggufTypeName(invalidNative)  == "unknown" );
    REQUIRE(ggufTypeName(invalidWrapped) == "unknown" );
}

TEST_CASE("JobGguf reports missing keys without fabricating values", "[gguf][edge][kv][missing]")
{
    JobGguf gguf;
    REQUIRE_FALSE(gguf.hasKey("job.missing"));
    REQUIRE(gguf.keyValue("job.missing") == nullptr);
}

TEST_CASE("JobGguf reader rejects an empty path", "[gguf][edge][reader][path]")
{
    JobGguf gguf;

    REQUIRE_FALSE(gguf.open(std::filesystem::path{}));
    REQUIRE(gguf.hasError());
    REQUIRE_FALSE(gguf.errorString().empty());
}

TEST_CASE("JobGguf reader rejects a missing file", "[gguf][edge][reader][path]")
{
    const std::filesystem::path missingPath = std::filesystem::temp_directory_path() /  "job_gguf_also_euler_is_rk4_drunk_cousin.gguf";
    std::error_code errorCode;
    std::filesystem::remove(missingPath, errorCode);

    JobGguf gguf;

    REQUIRE_FALSE(gguf.open(missingPath));
    REQUIRE(gguf.hasError());
    REQUIRE_FALSE(gguf.errorString().empty());
}

TEST_CASE("JobGguf reader rejects null and empty buffers", "[gguf][edge][reader][buffer]")
{
    JobGguf gguf;
    REQUIRE_FALSE(gguf.open(nullptr, 64));
    REQUIRE(gguf.hasError());

    const std::array<std::byte, 1> data{};
    REQUIRE_FALSE(gguf.open(data.data(), 0));
    REQUIRE(gguf.hasError());
    REQUIRE_FALSE(gguf.open(std::span<const std::byte>{}));
    REQUIRE(gguf.hasError());
}

TEST_CASE("JobGguf reader rejects a null FILE pointer", "[gguf][edge][reader][file_ptr]")
{
    JobGguf gguf;
    REQUIRE_FALSE(gguf.open(static_cast<std::FILE *>(nullptr)));
    REQUIRE(gguf.hasError());
}

TEST_CASE("JobGguf reader rejects an empty callback", "[gguf][edge][reader][callback]")
{
    JobGguf gguf;
    REQUIRE_FALSE(gguf.open(JobGgufReader::ReadCallback{}, 64, 1024));
    REQUIRE(gguf.hasError());
}

TEST_CASE("JobGguf reader rejects a zero callback source size", "[gguf][edge][reader][callback]")
{
    JobGguf gguf;
    JobGgufReader::ReadCallback callback = [](void *, std::uint64_t, std::size_t) -> std::size_t {
        return 0;
    };
    REQUIRE_FALSE(gguf.open(std::move(callback), 64, 0));
    REQUIRE(gguf.hasError());
}

TEST_CASE("Failed reads preserve the previously loaded GGUF context", "[gguf][edge][reader][transaction]")
{
    JobGguf gguf;
    populateExampleMetadata(gguf);
    JobGgufContext *contextBefore = gguf.context();
    struct gguf_context *nativeBefore = gguf.context()->context();
    const std::int64_t keyCountBefore = gguf.keyValueCount();
    REQUIRE_FALSE(gguf.open(std::filesystem::path{}));
    REQUIRE(gguf.context() == contextBefore);
    REQUIRE(gguf.context()->context() == nativeBefore);
    REQUIRE(gguf.keyValueCount() == keyCountBefore);
    REQUIRE(gguf.hasKey("general.architecture"));
}

TEST_CASE("JobGguf writer rejects a null FILE pointer", "[gguf][edge][writer][file_ptr]")
{
    JobGguf gguf;
    populateExampleMetadata(gguf);
    REQUIRE_FALSE(gguf.save(static_cast<std::FILE *>(nullptr)));
    REQUIRE(gguf.hasError());
}

TEST_CASE("JobGguf writer rejects an empty output path", "[gguf][edge][writer][path]")
{
    JobGguf gguf;
    populateExampleMetadata(gguf);
    REQUIRE_FALSE(gguf.save(std::filesystem::path{}));
    REQUIRE(gguf.hasError());
}

TEST_CASE("JobGguf writer rejects an undersized metadata destination", "[gguf][edge][writer][metadata]")
{
    JobGguf gguf;
    populateExampleMetadata(gguf);

    const std::size_t requiredSize = gguf.metadataSize();
    REQUIRE(requiredSize > 1);

    std::vector<std::byte> destination(requiredSize - 1);
    REQUIRE_FALSE(gguf.writeMetadata(
        destination.data(),
        destination.size()
        ));

    REQUIRE(gguf.hasError());
}

TEST_CASE("JobGguf writer rejects a null metadata destination", "[gguf][edge][writer][metadata]")
{
    JobGguf gguf;
    populateExampleMetadata(gguf);
    REQUIRE_FALSE(gguf.writeMetadata(nullptr, gguf.metadataSize()));
    REQUIRE(gguf.hasError());
}

TEST_CASE("JobGguf reset preserves the stable context wrapper", "[gguf][edge][reset][ownership]")
{
    JobGguf gguf;
    populateExampleMetadata(gguf);

    JobGgufContext *contextBefore       = gguf.context();
    JobGgufReader  *readerBefore        = gguf.reader();
    JobGgufWriter  *writerBefore        = gguf.writer();
    struct gguf_context *nativeBefore   = gguf.context()->context();
    REQUIRE(gguf.hasContent());

    gguf.reset();
    REQUIRE(gguf.context() == contextBefore);
    REQUIRE(gguf.reader() == readerBefore);
    REQUIRE(gguf.writer() == writerBefore);

    REQUIRE(gguf.context()->context() != nativeBefore);
    REQUIRE(gguf.context()->isValid());
    REQUIRE(gguf.reader()->isValid());
    REQUIRE(gguf.writer()->isValid());

    REQUIRE_FALSE(gguf.hasContent());
    REQUIRE(gguf.keyValueCount() == 0);
    REQUIRE(gguf.tensorCount() == 0);
    REQUIRE_FALSE(gguf.hasError());
}

TEST_CASE("JobGguf context output can be replaced", "[gguf][edge][ownership][context_output]")
{
    JobGgmlContext::UPtr firstContext;
    JobGgmlContext::UPtr secondContext;

    JobGguf gguf{ &firstContext };
    REQUIRE(gguf.contextOutput() == &firstContext);

    gguf.setContextOutput(&secondContext);
    REQUIRE(gguf.contextOutput() == &secondContext);

    gguf.setContextOutput(nullptr);
    REQUIRE(gguf.contextOutput() == nullptr);
}

TEST_CASE("JobGguf clearError clears subsystem error state", "[gguf][edge][error]")
{
    JobGguf gguf;
    REQUIRE_FALSE(gguf.open(std::filesystem::path{}));

    REQUIRE(gguf.hasError());
    REQUIRE(gguf.reader()->hasError());

    gguf.clearError();

    REQUIRE_FALSE(gguf.hasError());
    REQUIRE_FALSE(gguf.reader()->hasError());
    REQUIRE_FALSE(gguf.writer()->hasError());
}


// Block three: benchmarks / stress
#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("GGUF key lookup performance", "[gguf][benchmark][lookup]")
{
    JobGguf gguf;
    constexpr std::size_t keyCount = 1024;
    for (std::size_t index = 0; index < keyCount; ++index)
        gguf.setKeyValue(JobGgufKv{ "job.key." + std::to_string(index), static_cast<std::uint32_t>(index) });

    REQUIRE(gguf.keyValueCount() == static_cast<std::int64_t>(keyCount));
    BENCHMARK("lookup one GGUF key"){
        return gguf.hasKey("job.key.777");
    };
}

TEST_CASE("GGUF typed value reconstruction performance", "[gguf][benchmark][kv]")
{
    JobGguf gguf;
    populateExampleMetadata(gguf);
    BENCHMARK("reconstruct one typed GGUF value"){
        return gguf.keyValue("job.dimensions");
    };
}

TEST_CASE("GGUF metadata serialization performance", "[gguf][benchmark][metadata]")
{
    JobGguf gguf;
    constexpr std::size_t keyCount = 1024;
    for (std::size_t index = 0; index < keyCount; ++index)
        gguf.setKeyValue(JobGgufKv{ "job.metadata." + std::to_string(index), static_cast<std::uint64_t>(index) });

    REQUIRE(gguf.metadataSize() > 0);
    BENCHMARK("serialize GGUF metadata") {
        return gguf.metadata();
    };
}

TEST_CASE("GGUF memory buffer parsing performance", "[gguf][benchmark][reader]")
{
    TransientTestFile temporaryFile{
        transientPath("job_gguf_buffer_benchmark.gguf")
    };

    JobGguf source;
    populateExampleMetadata(source);
    REQUIRE(source.save( temporaryFile.path()));
    const std::vector<std::byte> fileData = readFileBytes( temporaryFile.path() );

    REQUIRE_FALSE(fileData.empty());
    BENCHMARK("parse small GGUF memory buffer") {
        JobGguf destination;
        destination.initParams()->setNoAlloc(true);
        destination.initParams()->setCreateContext(false);
        return destination.open(std::span<const std::byte>{fileData.data(), fileData.size()});
    };
}
#endif