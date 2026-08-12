#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_ggml_quantization_params.h>
#include <job_ggml_quantization_result.h>
#include <job_ggml_quantizer.h>

using namespace job::ggml;

// ============================================================================
// Block one: usage / examples
// ============================================================================

TEST_CASE("Quantizer converts F32 rows into GGML quantized storage",
          "[ggml][quant][usage][chunk]")
{
    constexpr JobGgmlType QuantType = JobGgmlType::Q8_0;
    constexpr std::int64_t RowCount = 4;

    const std::int64_t elementsPerRow = JobGgmlQuantizer::blockSize(QuantType);
    REQUIRE(elementsPerRow > 0);

    JobGgmlQuantizationParams params{
        QuantType,
        0,
        RowCount,
        elementsPerRow
    };

    const std::size_t sourceElements =
        static_cast<std::size_t>(RowCount * elementsPerRow);

    std::vector<float> source(sourceElements);

    for (std::size_t i = 0; i < source.size(); ++i)
        source[i] = static_cast<float>(i % 17) / 17.0f;

    const std::size_t requiredBytes = JobGgmlQuantizer::requiredBytes(params);
    REQUIRE(requiredBytes > 0);

    std::vector<std::byte> destination(requiredBytes);

    const JobGgmlQuantizationResult result =
        JobGgmlQuantizer::quantizeChunk(params, source, destination);

    REQUIRE(result.type() == QuantType);
    REQUIRE(result.rowsProcessed() == RowCount);
    REQUIRE(result.bytesWritten() == requiredBytes);
}

TEST_CASE("Quantization parameters can attach an importance matrix",
          "[ggml][quant][usage][imatrix]")
{
    JobGgmlQuantizationParams params{
        JobGgmlType::Q8_0,
        0,
        1,
        JobGgmlQuantizer::blockSize(JobGgmlType::Q8_0)
    };

    REQUIRE_FALSE(params.hasImportanceMatrix());

    const std::array<float, 8> importance{
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f
    };

    params.setImportanceMatrix(importance);

    REQUIRE(params.hasImportanceMatrix());
    REQUIRE(params.importanceMatrix().size() == importance.size());
    REQUIRE(params.importanceMatrix().data() == importance.data());

    params.clearImportanceMatrix();
    REQUIRE_FALSE(params.hasImportanceMatrix());
}

TEST_CASE("Quantizer exposes GGML quantization type information",
          "[ggml][quant][usage][traits]")
{
    REQUIRE(JobGgmlQuantizer::isQuantized(JobGgmlType::Q8_0));
    REQUIRE_FALSE(JobGgmlQuantizer::isQuantized(JobGgmlType::F32));

    const std::int64_t blockSize = JobGgmlQuantizer::blockSize(JobGgmlType::Q8_0);
    REQUIRE(blockSize > 0);

    REQUIRE(JobGgmlQuantizer::typeSize(JobGgmlType::Q8_0) > 0);
    REQUIRE(JobGgmlQuantizer::rowSize(JobGgmlType::Q8_0, blockSize) > 0);
}

// ============================================================================
// Block two: edge cases / invariants
// ============================================================================

TEST_CASE("Quantizer rejects invalid quantization buffer ranges",
          "[ggml][quant][edge][buffers]")
{
    constexpr JobGgmlType QuantType = JobGgmlType::Q8_0;

    const std::int64_t elementsPerRow = JobGgmlQuantizer::blockSize(QuantType);
    REQUIRE(elementsPerRow > 0);

    JobGgmlQuantizationParams params{
        QuantType,
        0,
        2,
        elementsPerRow
    };

    const std::size_t sourceElements =
        static_cast<std::size_t>(params.rows() * params.elementsPerRow());

    const std::size_t destinationBytes =
        JobGgmlQuantizer::requiredBytes(params);

    REQUIRE(destinationBytes > 0);

    std::vector<float> source(sourceElements, 0.25f);
    std::vector<std::byte> destination(destinationBytes);

    REQUIRE(JobGgmlQuantizer::validate(params, source, destination));

    std::vector<float> shortSource(sourceElements - 1, 0.25f);
    CHECK_FALSE(JobGgmlQuantizer::validate(params, shortSource, destination));

    std::vector<std::byte> shortDestination(destinationBytes - 1);
    CHECK_FALSE(JobGgmlQuantizer::validate(params, source, shortDestination));
}

TEST_CASE("Quantizer rejects invalid row and block alignment",
          "[ggml][quant][edge][alignment]")
{
    constexpr JobGgmlType QuantType = JobGgmlType::Q8_0;

    const std::int64_t blockSize = JobGgmlQuantizer::blockSize(QuantType);
    REQUIRE(blockSize > 1);

    JobGgmlQuantizationParams params{
        QuantType,
        0,
        2,
        blockSize
    };

    const std::size_t sourceElements =
        static_cast<std::size_t>(params.rows() * params.elementsPerRow());

    std::vector<float> source(sourceElements, 0.25f);
    std::vector<std::byte> destination(JobGgmlQuantizer::requiredBytes(params));

    REQUIRE(JobGgmlQuantizer::validate(params, source, destination));

    params.setStart(1);
    CHECK_FALSE(JobGgmlQuantizer::validate(params, source, destination));

    params.setStart(0);
    params.setElementsPerRow(blockSize - 1);
    CHECK_FALSE(JobGgmlQuantizer::validate(params, source, destination));
}

TEST_CASE("Quantizer rejects missing importance matrix when required",
          "[ggml][quant][edge][imatrix]")
{
    // Find a supported GGML quantized type that requires an importance matrix.
    constexpr std::array<JobGgmlType, 8> candidateTypes{
        JobGgmlType::IQ2_XXS,
        JobGgmlType::IQ2_XS,
        JobGgmlType::IQ3_XXS,
        JobGgmlType::IQ1_S,
        JobGgmlType::IQ3_S,
        JobGgmlType::IQ2_S,
        JobGgmlType::IQ1_M,
        JobGgmlType::IQ4_XS
    };

    JobGgmlType requiredType{};
    bool found = false;

    for (const JobGgmlType type : candidateTypes) {
        if (JobGgmlQuantizer::requiresImportanceMatrix(type)) {
            requiredType = type;
            found = true;
            break;
        }
    }

    if (!found)
        SKIP("No tested GGML quantization type requires an importance matrix");

    const std::int64_t elementsPerRow = JobGgmlQuantizer::blockSize(requiredType);
    REQUIRE(elementsPerRow > 0);

    JobGgmlQuantizationParams params{
        requiredType,
        0,
        1,
        elementsPerRow
    };

    std::vector<float> source(static_cast<std::size_t>(elementsPerRow), 0.25f);
    std::vector<std::byte> destination(JobGgmlQuantizer::requiredBytes(params));

    REQUIRE_FALSE(destination.empty());
    CHECK_FALSE(JobGgmlQuantizer::validate(params, source, destination));
}

TEST_CASE("Quantizer rejects unusable quantization dimensions",
          "[ggml][quant][edge][dimensions]")
{
    JobGgmlQuantizationParams params{JobGgmlType::Q8_0};

    CHECK(JobGgmlQuantizer::requiredBytes(params) == 0);

    params.setRows(1);
    CHECK(JobGgmlQuantizer::requiredBytes(params) == 0);

    params.setElementsPerRow(1);
    CHECK(JobGgmlQuantizer::requiredBytes(params) == 0);
}

// ============================================================================
// Block three: benchmarks / stress
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Quantization chunk conversion performance",
          "[ggml][quant][benchmark]")
{
    constexpr JobGgmlType QuantType = JobGgmlType::Q8_0;
    constexpr std::int64_t RowCount = 1024;

    const std::int64_t elementsPerRow = JobGgmlQuantizer::blockSize(QuantType);
    REQUIRE(elementsPerRow > 0);

    JobGgmlQuantizationParams params{
        QuantType,
        0,
        RowCount,
        elementsPerRow
    };

    const std::size_t sourceElements =
        static_cast<std::size_t>(RowCount * elementsPerRow);

    std::vector<float> source(sourceElements, 0.25f);
    std::vector<std::byte> destination(JobGgmlQuantizer::requiredBytes(params));

    REQUIRE_FALSE(destination.empty());

    BENCHMARK("quantize 1024 Q8_0 rows") {
        return JobGgmlQuantizer::quantizeChunk(params, source, destination);
    };
}

TEST_CASE("Quantization repeated validation stress",
          "[ggml][quant][stress][validate]")
{
    constexpr std::size_t IterationCount = 10000;
    constexpr JobGgmlType QuantType = JobGgmlType::Q8_0;

    const std::int64_t elementsPerRow = JobGgmlQuantizer::blockSize(QuantType);
    REQUIRE(elementsPerRow > 0);

    JobGgmlQuantizationParams params{
        QuantType,
        0,
        4,
        elementsPerRow
    };

    const std::size_t sourceElements =
        static_cast<std::size_t>(params.rows() * params.elementsPerRow());

    std::vector<float> source(sourceElements, 0.25f);
    std::vector<std::byte> destination(JobGgmlQuantizer::requiredBytes(params));

    REQUIRE_FALSE(destination.empty());

    BENCHMARK("validate quantization buffers 10000 times") {
        for (std::size_t iteration = 0; iteration < IterationCount; ++iteration) {
            if (!JobGgmlQuantizer::validate(params, source, destination))
                FAIL("Quantization validation failed during stress benchmark");
        }
    };
}

#endif