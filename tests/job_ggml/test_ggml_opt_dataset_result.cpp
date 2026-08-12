#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_ggml_context.h>
#include <job_ggml_opt_dataset.h>
#include <job_ggml_opt_result.h>
#include <job_ggml_tensor.h>

#include <real_type.h>

#include "test_ggml_utils.h"

using namespace job::ggml;
using Catch::Approx;


// Block one: usage / examples
TEST_CASE("Optimization dataset owns labeled data and label tensors", "[ggml][opt][dataset][usage][labeled]")
{
    CpuOptDatasetFixture fixture;

    JobGgmlOptDataset &dataset = fixture.dataset;
    REQUIRE(dataset.isValid());
    REQUIRE(dataset.hasLabels());

    REQUIRE(dataset.neDatapoint() == CpuOptDatasetFixture::NeDatapoint);
    REQUIRE(dataset.neLabel() == CpuOptDatasetFixture::NeLabel);
    REQUIRE(dataset.ndata() == CpuOptDatasetFixture::Ndata);
    REQUIRE(dataset.ndataShard() == CpuOptDatasetFixture::NdataShard);
    REQUIRE(dataset.shardCount() == CpuOptDatasetFixture::Ndata / CpuOptDatasetFixture::NdataShard);
    REQUIRE(dataset.dataType() == JobGgmlType::F32);
    REQUIRE(dataset.ggmlDataType() == GGML_TYPE_F32);
    REQUIRE(dataset.labelType() == JobGgmlType::F32);
    REQUIRE(dataset.ggmlLabelType() == GGML_TYPE_F32);
    REQUIRE(dataset.dataset() != nullptr);
    REQUIRE(dataset.data() != nullptr);
    REQUIRE(dataset.labels() != nullptr);

    REQUIRE(dataset.data()->isValid());
    REQUIRE(dataset.labels()->isValid());

    REQUIRE(dataset.data()->rank() == 2);

    REQUIRE(dataset.data()->extent(0) == CpuOptDatasetFixture::NeDatapoint);
    REQUIRE(dataset.data()->extent(1) == CpuOptDatasetFixture::Ndata);

    REQUIRE(dataset.labels()->rank() == 2);

    REQUIRE(dataset.labels()->extent(0) == CpuOptDatasetFixture::NeLabel);
    REQUIRE(dataset.labels()->extent(1) == CpuOptDatasetFixture::Ndata);
    REQUIRE(dataset.nbsData() == dataset.data()->byteCount() *
                                     static_cast<std::size_t>(CpuOptDatasetFixture::NdataShard) / static_cast<std::size_t>(CpuOptDatasetFixture::Ndata));

    REQUIRE(dataset.nbsLabels() == dataset.labels()->byteCount() *
                                       static_cast<std::size_t>(CpuOptDatasetFixture::NdataShard) / static_cast<std::size_t>(CpuOptDatasetFixture::Ndata));

    REQUIRE(dataset.data()->hasData());
    REQUIRE(dataset.labels()->hasData());

    REQUIRE(dataset.data()->dataPointer() != nullptr);
    REQUIRE(dataset.labels()->dataPointer() != nullptr);
}

TEST_CASE("Optimization dataset may omit labels", "[ggml][opt][dataset][usage][unlabeled]")
{
    JobGgmlOptDataset dataset{
        JobGgmlType::F32,
        JobGgmlType::F32,
        3,
        0,
        8,
        4
    };

    REQUIRE(dataset.isValid());
    REQUIRE_FALSE(dataset.hasLabels());

    REQUIRE(dataset.neDatapoint() == 3);
    REQUIRE(dataset.neLabel() == 0);
    REQUIRE(dataset.ndata() == 8);
    REQUIRE(dataset.ndataShard() == 4);
    REQUIRE(dataset.shardCount() == 2);

    REQUIRE(dataset.data() != nullptr);
    REQUIRE(dataset.data()->isValid());
    REQUIRE(dataset.labels() == nullptr);

    REQUIRE(dataset.data()->rank() == 2);
    REQUIRE(dataset.data()->extent(0) == 3);
    REQUIRE(dataset.data()->extent(1) == 8);
    REQUIRE(dataset.data()->elementCount() == 24);

    REQUIRE(dataset.nbsData() == dataset.data()->byteCount() / 2);
    REQUIRE(dataset.nbsLabels() == 0);
}

TEST_CASE("Optimization dataset exposes stable borrowed tensor wrappers", "[ggml][opt][dataset][usage][ownership]")
{
    CpuOptDatasetFixture fixture;

    JobGgmlOptDataset &dataset = fixture.dataset;
    JobGgmlTensor *firstData = dataset.data();
    JobGgmlTensor *secondData = dataset.data();
    JobGgmlTensor *firstLabels = dataset.labels();
    JobGgmlTensor *secondLabels = dataset.labels();
    REQUIRE(firstData != nullptr);
    REQUIRE(firstLabels != nullptr);

    REQUIRE(firstData == secondData);
    REQUIRE(firstLabels == secondLabels);

    REQUIRE(firstData->tensor() != nullptr);
    REQUIRE(firstLabels->tensor() != nullptr);

    REQUIRE(firstData->tensor() == ggml_opt_dataset_data(dataset.dataset()));
    REQUIRE(firstLabels->tensor() == ggml_opt_dataset_labels(dataset.dataset()));
}

TEST_CASE("Optimization dataset reports native shard byte counts", "[ggml][opt][dataset][usage][shard_bytes]")
{
    CpuOptDatasetFixture fixture;

    JobGgmlOptDataset &dataset = fixture.dataset;
    const std::size_t expectedDataBytes = static_cast<std::size_t>(CpuOptDatasetFixture::NeDatapoint * CpuOptDatasetFixture::NdataShard) * sizeof(float);
    const std::size_t expectedLabelBytes = static_cast<std::size_t>(CpuOptDatasetFixture::NeLabel * CpuOptDatasetFixture::NdataShard) * sizeof(float);
    REQUIRE(dataset.nbsData() == expectedDataBytes);
    REQUIRE(dataset.nbsLabels() == expectedLabelBytes);
}

TEST_CASE("Optimization dataset extracts one labeled shard to host memory", "[ggml][opt][dataset][usage][host_batch]")
{
    CpuOptDatasetFixture fixture;

    JobGgmlOptDataset &dataset = fixture.dataset;
    populateLabeledDataset(dataset);
    constexpr std::size_t dataElementCount = static_cast<std::size_t>(CpuOptDatasetFixture::NeDatapoint * CpuOptDatasetFixture::NdataShard);

    constexpr std::size_t labelElementCount = static_cast<std::size_t>(CpuOptDatasetFixture::NeLabel * CpuOptDatasetFixture::NdataShard);

    std::array<float, dataElementCount> dataBatch{};
    std::array<float, labelElementCount> labelBatch{};

    dataset.getBatchHost(dataBatch.data(),
                         floatByteCount(dataBatch.size()),
                         labelBatch.data(),
                         0);

    for (std::size_t index = 0; index < dataBatch.size(); ++index)
        REQUIRE(dataBatch[index] == Approx(static_cast<float>(index + 1)));


    for (std::size_t index = 0; index < labelBatch.size(); ++index)
        REQUIRE(labelBatch[index] == Approx(static_cast<float>(100 + index)));

}

TEST_CASE("Optimization dataset extracts successive labeled host batches", "[ggml][opt][dataset][usage][host_batch][sequence]")
{
    CpuOptDatasetFixture fixture;

    JobGgmlOptDataset &dataset = fixture.dataset;
    populateLabeledDataset(dataset);
    constexpr std::size_t dataElementCount = static_cast<std::size_t>(CpuOptDatasetFixture::NeDatapoint * CpuOptDatasetFixture::NdataShard);
    constexpr std::size_t labelElementCount = static_cast<std::size_t>(CpuOptDatasetFixture::NeLabel * CpuOptDatasetFixture::NdataShard);

    std::array<float, dataElementCount> firstData{};
    std::array<float, labelElementCount> firstLabels{};

    std::array<float, dataElementCount> secondData{};
    std::array<float, labelElementCount> secondLabels{};

    dataset.getBatchHost(firstData.data(),
                         floatByteCount(firstData.size()),
                         firstLabels.data(),
                         0);

    dataset.getBatchHost(secondData.data(),
                         floatByteCount(secondData.size()),
                         secondLabels.data(),
                         1);

    for (std::size_t index = 0; index < firstData.size(); ++index) {
        REQUIRE(firstData[index] == Approx(static_cast<float>(index + 1)));
        REQUIRE(secondData[index] == Approx(static_cast<float>(firstData.size() + index + 1)));
    }

    for (std::size_t index = 0; index < firstLabels.size(); ++index) {
        REQUIRE(firstLabels[index] == Approx(static_cast<float>(100 + index)));
        REQUIRE(secondLabels[index] == Approx(static_cast<float>(100 + firstLabels.size() + index)));
    }
}

TEST_CASE("Unlabeled optimization dataset extracts host data without labels", "[ggml][opt][dataset][usage][host_batch][unlabeled]")
{
    constexpr std::int64_t neDatapoint = 3;
    constexpr std::int64_t ndata       = 8;
    constexpr std::int64_t ndataShard  = 4;

    JobGgmlOptDataset dataset{
        JobGgmlType::F32,
        JobGgmlType::F32,
        neDatapoint,
        0,
        ndata,
        ndataShard
    };

    populateUnlabeledDataset(dataset);
    constexpr std::size_t batchElementCount = static_cast<std::size_t>(neDatapoint * ndataShard);
    std::array<float, batchElementCount> batch{};

    dataset.getBatchHost(batch.data(), floatByteCount(batch.size()), nullptr, 0);
    for (std::size_t index = 0; index < batch.size(); ++index)
        REQUIRE(batch[index] == Approx(static_cast<float>(index + 1)));

}

TEST_CASE("Optimization dataset extracts one labeled batch into tensors", "[ggml][opt][dataset][usage][tensor_batch]")
{
    CpuSchedulerFixture schedulerFixture;
    CpuOptDatasetFixture datasetFixture;

    JobGgmlOptDataset &dataset = datasetFixture.dataset;

    populateLabeledDataset(dataset);
    auto context = JobGgmlContext::createUniqMetadata(2);
    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    auto dataBatch = context->newTensor2d(JobGgmlType::F32, CpuOptDatasetFixture::NeDatapoint, CpuOptDatasetFixture::NdataShard);
    auto labelBatch = context->newTensor2d(JobGgmlType::F32, CpuOptDatasetFixture::NeLabel, CpuOptDatasetFixture::NdataShard);

    REQUIRE(dataBatch != nullptr);
    REQUIRE(labelBatch != nullptr);

    schedulerFixture.scheduler()->setTensorBackend(*dataBatch, *schedulerFixture.backend());
    schedulerFixture.scheduler()->setTensorBackend(*labelBatch, *schedulerFixture.backend());
    auto graph = context->newGraph();

    REQUIRE(graph != nullptr);

    graph->buildForwardExpand(*dataBatch);
    graph->buildForwardExpand(*labelBatch);
    schedulerFixture.scheduler()->splitGraph(*graph);
    REQUIRE(schedulerFixture.scheduler()->allocateGraph(*graph));

    REQUIRE(dataBatch->buffer() != nullptr);
    REQUIRE(labelBatch->buffer() != nullptr);

    dataset.getBatch(*dataBatch, labelBatch.get(), 0);
    std::array<float, static_cast<std::size_t>(CpuOptDatasetFixture::NeDatapoint * CpuOptDatasetFixture::NdataShard)> dataValues{};
    std::array<float, static_cast<std::size_t>(CpuOptDatasetFixture::NeLabel * CpuOptDatasetFixture::NdataShard)> labelValues{};

    schedulerFixture.backend()->getTensorAsync(*dataBatch,
                                               dataValues.data(),
                                               0,
                                               floatByteCount(dataValues.size()));

    schedulerFixture.backend()->getTensorAsync(*labelBatch,
                                               labelValues.data(),
                                               0,
                                               floatByteCount(labelValues.size()));

    schedulerFixture.backend()->synchronize();

    for (std::size_t index = 0; index < dataValues.size(); ++index)
        REQUIRE(dataValues[index] == Approx(static_cast<float>(index + 1)));


    for (std::size_t index = 0; index < labelValues.size(); ++index)
        REQUIRE(labelValues[index] == Approx(static_cast<float>(100 + index)));

}

TEST_CASE("Unlabeled optimization dataset extracts one batch into a tensor", "[ggml][opt][dataset][usage][tensor_batch][unlabeled]")
{
    constexpr std::int64_t neDatapoint = 3;
    constexpr std::int64_t ndata       = 8;
    constexpr std::int64_t ndataShard  = 4;

    CpuSchedulerFixture schedulerFixture;

    JobGgmlOptDataset dataset{
        JobGgmlType::F32,
        JobGgmlType::F32,
        neDatapoint,
        0,
        ndata,
        ndataShard
    };

    populateUnlabeledDataset(dataset);
    auto context = JobGgmlContext::createUniqMetadata(1);
    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    auto dataBatch = context->newTensor2d(JobGgmlType::F32, neDatapoint, ndataShard);

    REQUIRE(dataBatch != nullptr);

    schedulerFixture.scheduler()->setTensorBackend(*dataBatch,
                                                   *schedulerFixture.backend());

    auto graph = context->newGraph();

    REQUIRE(graph != nullptr);

    graph->buildForwardExpand(*dataBatch);
    schedulerFixture.scheduler()->splitGraph(*graph);
    REQUIRE(schedulerFixture.scheduler()->allocateGraph(*graph));
    REQUIRE(dataBatch->buffer() != nullptr);

    dataset.getBatch(*dataBatch, nullptr, 0);
    std::array<float, static_cast<std::size_t>(neDatapoint * ndataShard)> dataValues{};
    schedulerFixture.backend()->getTensorAsync(*dataBatch,
                                               dataValues.data(),
                                               0,
                                               floatByteCount(dataValues.size()));

    schedulerFixture.backend()->synchronize();

    for (std::size_t index = 0; index < dataValues.size(); ++index)
        REQUIRE(dataValues[index] == Approx(static_cast<float>(index + 1)));
}

TEST_CASE("Optimization result begins valid and empty", "[ggml][opt][result][usage]")
{
    JobGgmlOptResult result;

    REQUIRE(result.isValid());
    REQUIRE(result.result() != nullptr);
    REQUIRE(result.ndata() == 0);

    double uncertainty = 0.0;

    const double loss = result.loss(&uncertainty);
    REQUIRE(loss == Approx(0.0));

    REQUIRE_FALSE(job::core::isSafeFinite(static_cast<float>(uncertainty)));
    REQUIRE(result.predictions().empty());
}

TEST_CASE("Optimization result may be reset and reused while empty", "[ggml][opt][result][usage][reset]")
{
    JobGgmlOptResult result;

    ggml_opt_result_t nativeBefore = result.result();
    REQUIRE(nativeBefore != nullptr);

    result.reset();

    REQUIRE(result.isValid());
    REQUIRE(result.ndata() == 0);
    REQUIRE(result.predictions().empty());

    REQUIRE(result.result() == nativeBefore);
    result.reset();

    REQUIRE(result.isValid());
    REQUIRE(result.ndata() == 0);
    REQUIRE(result.predictions().empty());
    REQUIRE(result.result() == nativeBefore);
}

// Block two: edge cases / invariants
TEST_CASE("Optimization dataset rejects invalid dimensions", "[ggml][opt][dataset][edge][creation]")
{
    REQUIRE_THROWS_AS(( JobGgmlOptDataset{ JobGgmlType::F32, JobGgmlType::F32, 0, 1, 8, 4 }), std::invalid_argument);
    REQUIRE_THROWS_AS(( JobGgmlOptDataset{ JobGgmlType::F32, JobGgmlType::F32, -1, 1, 8, 4 }), std::invalid_argument);
    REQUIRE_THROWS_AS(( JobGgmlOptDataset{ JobGgmlType::F32, JobGgmlType::F32, 2, -1, 8, 4 }), std::invalid_argument);
    REQUIRE_THROWS_AS(( JobGgmlOptDataset{ JobGgmlType::F32, JobGgmlType::F32, 2, 1, 0, 4 }), std::invalid_argument);
    REQUIRE_THROWS_AS(( JobGgmlOptDataset{ JobGgmlType::F32, JobGgmlType::F32, 2, 1, -1, 4 }), std::invalid_argument);
    REQUIRE_THROWS_AS(( JobGgmlOptDataset{ JobGgmlType::F32, JobGgmlType::F32, 2, 1, 8, 0 }), std::invalid_argument);
    REQUIRE_THROWS_AS(( JobGgmlOptDataset{ JobGgmlType::F32, JobGgmlType::F32, 2, 1, 8, -1 }), std::invalid_argument);
}

TEST_CASE("Optimization dataset rejects unsupported tensor types", "[ggml][opt][dataset][edge][type]")
{
    REQUIRE_THROWS_AS(( JobGgmlOptDataset{ JobGgmlType::Count, JobGgmlType::F32, 2, 1, 8, 4 }), std::invalid_argument);
    REQUIRE_THROWS_AS(( JobGgmlOptDataset{ JobGgmlType::F32, JobGgmlType::Count, 2, 1, 8, 4 }), std::invalid_argument);
}

TEST_CASE("Optimization dataset requires complete shards", "[ggml][opt][dataset][edge][shard]")
{
    REQUIRE_THROWS_AS(( JobGgmlOptDataset{ JobGgmlType::F32, JobGgmlType::F32, 2, 1, 10, 4 }), std::invalid_argument);
    REQUIRE_THROWS_AS(( JobGgmlOptDataset{ JobGgmlType::F32, JobGgmlType::F32, 2, 1, 4, 8 }), std::invalid_argument);
}

TEST_CASE("Optimization dataset rejects an out of range host batch", "[ggml][opt][dataset][edge][host_batch][index]")
{
    CpuOptDatasetFixture fixture;

    constexpr std::size_t dataElementCount = static_cast<std::size_t>(CpuOptDatasetFixture::NeDatapoint * CpuOptDatasetFixture::NdataShard);
    constexpr std::size_t labelElementCount = static_cast<std::size_t>(CpuOptDatasetFixture::NeLabel * CpuOptDatasetFixture::NdataShard);
    std::array<float, dataElementCount> dataBatch{};
    std::array<float, labelElementCount> labelBatch{};

    REQUIRE_THROWS_AS(fixture.dataset.getBatchHost(dataBatch.data(),
                                                   floatByteCount(dataBatch.size()),
                                                   labelBatch.data(),
                                                   -1),
                      std::out_of_range);

    REQUIRE_THROWS_AS(fixture.dataset.getBatchHost(dataBatch.data(),
                                                   floatByteCount(dataBatch.size()),
                                                   labelBatch.data(),
                                                   fixture.dataset.shardCount()),
                      std::out_of_range);
}

TEST_CASE("Optimization dataset rejects invalid host batch storage", "[ggml][opt][dataset][edge][host_batch][storage]")
{
    CpuOptDatasetFixture fixture;

    constexpr std::size_t dataElementCount = static_cast<std::size_t>(CpuOptDatasetFixture::NeDatapoint * CpuOptDatasetFixture::NdataShard);
    constexpr std::size_t labelElementCount = static_cast<std::size_t>(CpuOptDatasetFixture::NeLabel * CpuOptDatasetFixture::NdataShard);
    std::array<float, dataElementCount> dataBatch{};
    std::array<float, labelElementCount> labelBatch{};

    REQUIRE_THROWS_AS(fixture.dataset.getBatchHost(nullptr,
                                                   floatByteCount(dataBatch.size()),
                                                   labelBatch.data(),
                                                   0),
                      std::invalid_argument);

    REQUIRE_THROWS_AS(fixture.dataset.getBatchHost(dataBatch.data(), 0, labelBatch.data(), 0), std::invalid_argument);
    REQUIRE_THROWS_AS(fixture.dataset.getBatchHost(dataBatch.data(),
                                                   floatByteCount(dataBatch.size()) - 1,
                                                   labelBatch.data(),
                                                   0),
                      std::invalid_argument);

    REQUIRE_THROWS_AS(fixture.dataset.getBatchHost(dataBatch.data(),
                                                   floatByteCount(dataBatch.size()),
                                                   nullptr,
                                                   0),
                      std::invalid_argument);
}

TEST_CASE("Unlabeled optimization dataset rejects an unexpected label destination", "[ggml][opt][dataset][edge][host_batch][labels]")
{
    JobGgmlOptDataset dataset{
        JobGgmlType::F32,
        JobGgmlType::F32,
        3,
        0,
        8,
        4
    };

    std::array<float, 12> dataBatch{};
    std::array<float, 4> unexpectedLabels{};

    REQUIRE_THROWS_AS(dataset.getBatchHost(dataBatch.data(),
                                           floatByteCount(dataBatch.size()),
                                           unexpectedLabels.data(),
                                           0),
                      std::invalid_argument);
}

TEST_CASE("Optimization dataset rejects incompatible tensor batch types", "[ggml][opt][dataset][edge][tensor_batch][type]")
{
    CpuOptDatasetFixture fixture;

    constexpr std::size_t f16DataBytes = static_cast<std::size_t>(CpuOptDatasetFixture::NeDatapoint * CpuOptDatasetFixture::NdataShard) * sizeof(ggml_fp16_t);
    constexpr std::size_t labelBytes = static_cast<std::size_t>(CpuOptDatasetFixture::NeLabel * CpuOptDatasetFixture::NdataShard) * sizeof(float);
    auto context = JobGgmlContext::createUniqHostContext(2, f16DataBytes + labelBytes);

    REQUIRE(context != nullptr);

    auto wrongDataType = context->newTensor2d(JobGgmlType::F16, CpuOptDatasetFixture::NeDatapoint, CpuOptDatasetFixture::NdataShard);
    auto labelBatch = context->newTensor2d(JobGgmlType::F32, CpuOptDatasetFixture::NeLabel, CpuOptDatasetFixture::NdataShard);
    REQUIRE(wrongDataType != nullptr);
    REQUIRE(labelBatch != nullptr);

    REQUIRE_THROWS_AS(fixture.dataset.getBatch(*wrongDataType,
                                               labelBatch.get(),
                                               0),
                      std::invalid_argument);
}

TEST_CASE("Optimization dataset rejects incompatible tensor batch shapes", "[ggml][opt][dataset][edge][tensor_batch][shape]")
{
    CpuOptDatasetFixture fixture;

    auto context = JobGgmlContext::createUniqHostContext(2, 4096);
    REQUIRE(context != nullptr);

    auto wrongDataShape = context->newTensor2d(JobGgmlType::F32, CpuOptDatasetFixture::NeDatapoint + 1, CpuOptDatasetFixture::NdataShard);
    auto labelBatch = context->newTensor2d(JobGgmlType::F32, CpuOptDatasetFixture::NeLabel, CpuOptDatasetFixture::NdataShard);

    REQUIRE(wrongDataShape != nullptr);
    REQUIRE(labelBatch != nullptr);

    REQUIRE_THROWS_AS(fixture.dataset.getBatch(*wrongDataShape,
                                               labelBatch.get(),
                                               0),
                      std::invalid_argument);
}

TEST_CASE("Optimization dataset rejects incompatible label batch shapes", "[ggml][opt][dataset][edge][tensor_batch][label_shape]")
{
    CpuOptDatasetFixture fixture;

    auto context = JobGgmlContext::createUniqHostContext(2, 4096);
    REQUIRE(context != nullptr);

    auto dataBatch = context->newTensor2d(JobGgmlType::F32, CpuOptDatasetFixture::NeDatapoint, CpuOptDatasetFixture::NdataShard);
    auto wrongLabelShape = context->newTensor2d(JobGgmlType::F32, CpuOptDatasetFixture::NeLabel + 1, CpuOptDatasetFixture::NdataShard);
    REQUIRE(dataBatch != nullptr);
    REQUIRE(wrongLabelShape != nullptr);

    REQUIRE_THROWS_AS(fixture.dataset.getBatch(*dataBatch,
                                               wrongLabelShape.get(),
                                               0),
                      std::invalid_argument);
}

TEST_CASE("Labeled optimization dataset requires a label batch tensor", "[ggml][opt][dataset][edge][tensor_batch][labels]")
{
    CpuOptDatasetFixture fixture;

    constexpr std::size_t dataElementCount = static_cast<std::size_t>(CpuOptDatasetFixture::NeDatapoint * CpuOptDatasetFixture::NdataShard);
    auto context = JobGgmlContext::createUniqHostContext(1, floatByteCount(dataElementCount));
    REQUIRE(context != nullptr);

    auto dataBatch = context->newTensor2d(JobGgmlType::F32, CpuOptDatasetFixture::NeDatapoint, CpuOptDatasetFixture::NdataShard);
    REQUIRE(dataBatch != nullptr);

    REQUIRE_THROWS_AS(fixture.dataset.getBatch(*dataBatch, nullptr, 0), std::invalid_argument);
}

TEST_CASE("Optimization dataset rejects an out of range tensor batch", "[ggml][opt][dataset][edge][tensor_batch][index]")
{
    CpuOptDatasetFixture fixture;

    auto context = JobGgmlContext::createUniqHostContext(2, 4096);
    REQUIRE(context != nullptr);

    auto dataBatch = context->newTensor2d(JobGgmlType::F32, CpuOptDatasetFixture::NeDatapoint, CpuOptDatasetFixture::NdataShard);
    auto labelBatch = context->newTensor2d(JobGgmlType::F32, CpuOptDatasetFixture::NeLabel, CpuOptDatasetFixture::NdataShard);
    REQUIRE(dataBatch != nullptr);
    REQUIRE(labelBatch != nullptr);

    REQUIRE_THROWS_AS(fixture.dataset.getBatch(*dataBatch,
                                               labelBatch.get(),
                                               -1),
                      std::out_of_range);

    REQUIRE_THROWS_AS(fixture.dataset.getBatch(*dataBatch,
                                               labelBatch.get(),
                                               fixture.dataset.shardCount()),
                      std::out_of_range);
}

TEST_CASE("Optimization result empty loss remains deterministic", "[ggml][opt][result][edge][empty]")
{
    JobGgmlOptResult result;

    double uncertainty = 123.0;
    const double loss = result.loss(&uncertainty);
    REQUIRE(loss == Approx(0.0));
    REQUIRE_FALSE(job::core::isSafeFinite(static_cast<float>(uncertainty)));

    REQUIRE(result.ndata() == 0);
    REQUIRE(result.predictions().empty());
}

// Block three: benchmarks / stress
#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Optimization dataset construction performance", "[ggml][opt][dataset][benchmark][construction]")
{
    BENCHMARK("construct labeled optimization dataset") {
        return JobGgmlOptDataset::createUniq(JobGgmlType::F32, JobGgmlType::F32, 16, 4, 1024, 32);
    };
}

TEST_CASE("Optimization dataset host batch extraction performance", "[ggml][opt][dataset][benchmark][host_batch]")
{
    CpuOptDatasetFixture fixture;

    populateLabeledDataset(fixture.dataset);
    constexpr std::size_t dataElementCount = static_cast<std::size_t>(CpuOptDatasetFixture::NeDatapoint * CpuOptDatasetFixture::NdataShard);
    constexpr std::size_t labelElementCount = static_cast<std::size_t>(CpuOptDatasetFixture::NeLabel * CpuOptDatasetFixture::NdataShard);
    std::array<float, dataElementCount> dataBatch{};
    std::array<float, labelElementCount> labelBatch{};

    BENCHMARK("extract one optimization dataset shard to host") {
        fixture.dataset.getBatchHost(dataBatch.data(),
                                     floatByteCount(dataBatch.size()),
                                     labelBatch.data(),
                                     0);

        return dataBatch[0];
    };
}

TEST_CASE("Optimization dataset tensor batch extraction performance", "[ggml][opt][dataset][benchmark][tensor_batch]")
{
    CpuSchedulerFixture schedulerFixture;
    CpuOptDatasetFixture datasetFixture;

    JobGgmlOptDataset &dataset = datasetFixture.dataset;

    populateLabeledDataset(dataset);
    auto context = JobGgmlContext::createUniqMetadata(2);
    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    auto dataBatch = context->newTensor2d(JobGgmlType::F32, CpuOptDatasetFixture::NeDatapoint, CpuOptDatasetFixture::NdataShard);
    auto labelBatch = context->newTensor2d(JobGgmlType::F32, CpuOptDatasetFixture::NeLabel, CpuOptDatasetFixture::NdataShard);

    REQUIRE(dataBatch != nullptr);
    REQUIRE(labelBatch != nullptr);

    schedulerFixture.scheduler()->setTensorBackend(*dataBatch, *schedulerFixture.backend());
    schedulerFixture.scheduler()->setTensorBackend(*labelBatch, *schedulerFixture.backend());
    auto graph = context->newGraph();
    REQUIRE(graph != nullptr);

    graph->buildForwardExpand(*dataBatch);
    graph->buildForwardExpand(*labelBatch);
    schedulerFixture.scheduler()->splitGraph(*graph);

    REQUIRE(schedulerFixture.scheduler()->allocateGraph(*graph));
    REQUIRE(dataBatch->buffer() != nullptr);
    REQUIRE(labelBatch->buffer() != nullptr);

    BENCHMARK("extract one optimization dataset shard to tensors") {
        dataset.getBatch(*dataBatch, labelBatch.get(), 0);
        return dataBatch->buffer();
    };
}

TEST_CASE("Optimization result reset performance", "[ggml][opt][result][benchmark][reset]") {
    JobGgmlOptResult result;
    BENCHMARK("reset optimization result") {
        result.reset();
        return result.result();
    };
}

TEST_CASE("Optimization result empty query performance", "[ggml][opt][result][benchmark][query]")
{
    JobGgmlOptResult result;
    BENCHMARK("query empty optimization result") {
        double uncertainty = 0.0;
        return result.loss(&uncertainty);
    };
}

TEST_CASE("Optimization dataset repeated host extraction stress", "[ggml][opt][dataset][benchmark][stress][host_batch]")
{
    constexpr std::size_t iterationCount = 10000;

    CpuOptDatasetFixture fixture;
    populateLabeledDataset(fixture.dataset);

    constexpr std::size_t dataElementCount = static_cast<std::size_t>(CpuOptDatasetFixture::NeDatapoint * CpuOptDatasetFixture::NdataShard);
    constexpr std::size_t labelElementCount = static_cast<std::size_t>(CpuOptDatasetFixture::NeLabel * CpuOptDatasetFixture::NdataShard);

    std::array<float, dataElementCount> dataBatch{};
    std::array<float, labelElementCount> labelBatch{};

    BENCHMARK("extract 10000 optimization dataset shards to host") {
        for (std::size_t iteration = 0; iteration < iterationCount; ++iteration) {
            const std::int64_t ibatch = static_cast<std::int64_t>(iteration % static_cast<std::size_t>(fixture.dataset.shardCount()));
            fixture.dataset.getBatchHost(dataBatch.data(),
                                         floatByteCount(dataBatch.size()),
                                         labelBatch.data(),
                                         ibatch);
        }

        return dataBatch[0] + labelBatch[0];
    };
}

#endif