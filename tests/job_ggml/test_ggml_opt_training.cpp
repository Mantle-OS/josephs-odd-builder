#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <ggml.h>
#include <ggml-backend.h>

#include <job_ggml_backend.h>
#include <job_ggml_context.h>
#include <job_ggml_opt.h>
#include <job_ggml_opt_context.h>
#include <job_ggml_opt_dataset.h>
#include <job_ggml_opt_epoch_progress.h>
#include <job_ggml_opt_optimizer_params.h>
#include <job_ggml_opt_optimizer_schedule.h>
#include <job_ggml_opt_params.h>
#include <job_ggml_opt_result.h>
#include <job_ggml_opt_step_info.h>
#include <job_ggml_tensor.h>

#include <real_type.h>

#include "test_ggml_utils.h"

using namespace job::ggml;
using Catch::Approx;

namespace job::ggml::test {

/*
 * Tiny linear-regression problem:
 *
 *     y = weight * x
 *
 * Dataset:
 *
 *     x = { -4, -3, -2, -1, 1, 2, 3, 4 }
 *     y = { -8, -6, -4, -2, 2, 4, 6, 8 }
 *
 * The model has one trainable scalar parameter. The expected learned value is:
 *
 *     weight = 2
 *
 * Physical batches contain two datapoints. The complete dataset therefore
 * contains four physical batches.
 */
struct CpuLinearTrainingFixture
{
    static constexpr std::int64_t NeDatapoint = 1;
    static constexpr std::int64_t NeLabel     = 1;

    static constexpr std::int64_t Ndata      = 8;
    static constexpr std::int64_t NdataBatch = 2;
    static constexpr std::int64_t NdataShard = NdataBatch;

    static constexpr std::int64_t PhysicalBatchCount = Ndata / NdataBatch;

    static constexpr std::size_t ComputeTensorCount = 128; // 256;

    static constexpr std::array<float, Ndata> InputValues{
        -4.0f, -3.0f, -2.0f, -1.0f, 1.0f, 2.0f, 3.0f, 4.0f
    };

    static constexpr std::array<float, Ndata> LabelValues{
        -8.0f, -6.0f, -4.0f, -2.0f, 2.0f, 4.0f, 6.0f, 8.0f
    };

    CpuSchedulerFixture schedulerFixture;

    JobGgmlContext::UPtr computeContext;

    /*
     * This buffer owns the backend storage assigned to every tensor created
     * inside computeContext. It must outlive all wrappers and optimization
     * contexts that borrow those tensors.
     */
    ggml_backend_buffer_t computeBuffer{nullptr};

    JobGgmlTensor::UPtr inputs;
    JobGgmlTensor::UPtr weight;
    JobGgmlTensor::UPtr repeatedWeight;
    JobGgmlTensor::UPtr outputs;

    JobGgmlOptDataset dataset{
        JobGgmlType::F32,
        JobGgmlType::F32,
        NeDatapoint,
        NeLabel,
        Ndata,
        NdataShard
    };

    JobGgmlOptOptimizerSchedule::Ptr optimizerSchedule;
    JobGgmlOptParams::UPtr           params;
    JobGgmlOptContext::UPtr          optContext;
    JobGgmlTensorOp::UPtr            tensorOp; // Owned
    explicit CpuLinearTrainingFixture(bool constructOptContext = false,
                                      JobGgmlOptOptimizerType optimizer = JobGgmlOptOptimizerType::Sgd,
                                      std::int32_t optimizerPeriod = 1,
                                      float learningRate = 1.0e-2f)
    {
        createModel();
        allocateModelStorage();
        populateDataset();
        setWeight(0.0f);

        if (constructOptContext)
            createOptimizationContext(optimizer, optimizerPeriod, learningRate);
    }

    ~CpuLinearTrainingFixture()
    {
        optContext.reset();
        params.reset();
        optimizerSchedule.reset();

        tensorOp.reset();
        outputs.reset();
        repeatedWeight.reset();
        weight.reset();
        inputs.reset();

        if (computeBuffer) {
            ggml_backend_buffer_free(computeBuffer);
            computeBuffer = nullptr;
        }

        computeContext.reset();
    }

    CpuLinearTrainingFixture(const CpuLinearTrainingFixture &) = delete;
    CpuLinearTrainingFixture &operator=(const CpuLinearTrainingFixture &) = delete;
    CpuLinearTrainingFixture(CpuLinearTrainingFixture &&) = delete;
    CpuLinearTrainingFixture &operator=(CpuLinearTrainingFixture &&) = delete;

    void createModel()
    {
        computeContext = createMetadataStaticOptContext(ComputeTensorCount);

        if (!computeContext || !computeContext->isValid())
            throw std::runtime_error{"Failed to create linear-training compute context"};

        inputs = computeContext->newTensor2d(JobGgmlType::F32, NeDatapoint, NdataBatch);
        weight = computeContext->newTensor1d(JobGgmlType::F32, 1);

        if (!inputs || !weight)
            throw std::runtime_error{"Failed to create linear-training input or weight tensor"};

        inputs->setName("linear.inputs");
        weight->setName("linear.weight");

        inputs->data()->addFlag(JobGgmlTensorFlag::Input);
        weight->data()->addFlag(JobGgmlTensorFlag::Param);

        tensorOp = JobGgmlTensorOp::createUniq(weight->tensor(), computeContext.get());

        if (!tensorOp || !tensorOp->isValid())
            throw std::runtime_error{"Failed to create linear-training tensor operation"};

        repeatedWeight = tensorOp->repeat(*inputs);

        if (!repeatedWeight || !repeatedWeight->isValid())
            throw std::runtime_error{"Failed to repeat the linear-training scalar weight"};

        repeatedWeight->setName("linear.repeated_weight");

        tensorOp = JobGgmlTensorOp::createUniq(inputs->tensor(), computeContext.get());

        if (!tensorOp || !tensorOp->isValid())
            throw std::runtime_error{"Failed to create linear-training output operation"};

        outputs = tensorOp->mul(*repeatedWeight);

        if (!outputs || !outputs->isValid())
            throw std::runtime_error{"Failed to create linear-training output tensor"};

        outputs->setName("linear.outputs");
        outputs->data()->addFlag(JobGgmlTensorFlag::Output);
    }

    void allocateModelStorage()
    {
        computeBuffer = ggml_backend_alloc_ctx_tensors(computeContext->context(), schedulerFixture.backend()->backend());

        if (!computeBuffer)
            throw std::runtime_error{"Failed to allocate CPU backend storage for linear-training tensors"};

        if (!inputs->buffer() || !weight->buffer() || !repeatedWeight->buffer() || !outputs->buffer())
            throw std::runtime_error{"Linear-training tensors do not expose backend buffers"};
    }

    void populateDataset()
    {
        JobGgmlTensor *dataTensor  = dataset.data();
        JobGgmlTensor *labelTensor = dataset.labels();

        if (!dataTensor || !labelTensor || !dataTensor->dataPointer() || !labelTensor->dataPointer()) {
            throw std::runtime_error{"Failed to access linear-training dataset storage"};
        }

        auto *data   = static_cast<float *>(dataTensor->dataPointer());
        auto *labels = static_cast<float *>(labelTensor->dataPointer());

        for (std::size_t index = 0; index < InputValues.size(); ++index) {
            data[index]   = InputValues[index];
            labels[index] = LabelValues[index];
        }
    }

    void createOptimizationContext(JobGgmlOptOptimizerType optimizer = JobGgmlOptOptimizerType::Sgd,
                                   std::int32_t optimizerPeriod = 1,
                                   float learningRate = 1.0e-2f)
    {
        optimizerSchedule = createSchedule(optimizer, learningRate);

        params = createDefaultOptParams(schedulerFixture, JobGgmlOptLossType::MeanSquaredError);

        if (!params) {
            throw std::runtime_error{"Failed to create linear-training optimization parameters"};
        }

        params->setComputeContext(computeContext.get());
        params->setInputs(inputs.get());
        params->setOutputs(outputs.get());
        params->setOptimizer(optimizer);
        params->setOptPeriod(optimizerPeriod);
        params->setOptimizerSchedule(optimizerSchedule);

        if (!params->isValid() || !params->usesStaticGraphs()) {
            throw std::runtime_error{"Failed to configure linear-training static optimization parameters"};
        }

        optContext = JobGgmlOptContext::createUniq(*params);

        if (!optContext || !optContext->isValid() || !optContext->usesStaticGraphs()) {
            throw std::runtime_error{"Failed to construct linear-training optimization context"};
        }
    }

    [[nodiscard]] static JobGgmlOptOptimizerSchedule::Ptr createSchedule(JobGgmlOptOptimizerType optimizer,
                                                                         float learningRate)
    {
        return JobGgmlOptOptimizerSchedule::createShared(
            [optimizer, learningRate](JobGgmlOptOptimizerParams &optimizerParams, std::int64_t) {
                if (optimizer == JobGgmlOptOptimizerType::Sgd) {
                    optimizerParams.sgd()->setAlpha(learningRate);
                    optimizerParams.sgd()->setWd(0.0f);
                    return;
                }

                optimizerParams.adamw()->setAlpha(learningRate);
                optimizerParams.adamw()->setBeta1(0.9f);
                optimizerParams.adamw()->setBeta2(0.999f);
                optimizerParams.adamw()->setEps(1.0e-8f);
                optimizerParams.adamw()->setWd(0.0f);
            });
    }

    void setWeight(float value)
    {
        schedulerFixture.backend()->setTensorAsync(*weight, &value, 0, sizeof(value));
        schedulerFixture.backend()->synchronize();
    }

    [[nodiscard]] float readWeight()
    {
        float value = 0.0f;
        schedulerFixture.backend()->getTensorAsync(*weight, &value, 0, sizeof(value));
        schedulerFixture.backend()->synchronize();
        return value;
    }
};

constexpr std::array<float, CpuLinearTrainingFixture::Ndata> CpuLinearTrainingFixture::InputValues;
constexpr std::array<float, CpuLinearTrainingFixture::Ndata> CpuLinearTrainingFixture::LabelValues;

} // namespace job::ggml::test

// ============================================================================
// Block one: usage / examples
// ============================================================================

TEST_CASE("Managed optimization epoch trains one scalar linear model",
          "[ggml][opt][training][usage][epoch][sgd]")
{
    test::CpuLinearTrainingFixture fixture{
        true, JobGgmlOptOptimizerType::Sgd, 1, 1.0e-2f
    };

    REQUIRE(fixture.optContext != nullptr);
    REQUIRE(fixture.optContext->isValid());

    const float initialWeight = fixture.readWeight();
    REQUIRE(initialWeight == Approx(0.0f));

    JobGgmlOptResult trainResult;

    std::int64_t trainingCallbackCount = 0;
    std::int64_t stepCallbackCount     = 0;

    std::int64_t lastTrainingBatch = 0;
    std::int64_t lastOptimizerStep = 0;
    std::int64_t lastScheduleCount = 0;

    JobGgmlOpt::epoch(
        *fixture.optContext,
        fixture.dataset,
        &trainResult,
        nullptr,
        -1,
        [&trainingCallbackCount, &lastTrainingBatch](const JobGgmlOptEpochProgress &progress) {
            REQUIRE(progress.isValid());
            REQUIRE(progress.isTraining());
            REQUIRE_FALSE(progress.isValidation());

            REQUIRE(progress.context() != nullptr);
            REQUIRE(progress.dataset() != nullptr);
            REQUIRE(progress.result() != nullptr);

            REQUIRE(progress.ibatch() >= 1);
            REQUIRE(progress.ibatch() <= progress.ibatchMax());
            REQUIRE(progress.progress() > 0.0);

            ++trainingCallbackCount;
            lastTrainingBatch = progress.ibatch();
        },
        {},
        [&stepCallbackCount, &lastOptimizerStep, &lastScheduleCount](const JobGgmlOptStepInfo &step) {
            REQUIRE(step.isValid());
            REQUIRE(step.epoch() == 0);
            REQUIRE(step.optimizerStep() >= 1);
            REQUIRE(step.callbackCount() >= 1);

            ++stepCallbackCount;
            lastOptimizerStep = step.optimizerStep();
            lastScheduleCount = step.callbackCount();
        });

    const float trainedWeight = fixture.readWeight();

    REQUIRE(trainingCallbackCount == test::CpuLinearTrainingFixture::PhysicalBatchCount);
    REQUIRE(stepCallbackCount == test::CpuLinearTrainingFixture::PhysicalBatchCount);
    REQUIRE(lastTrainingBatch == test::CpuLinearTrainingFixture::PhysicalBatchCount);
    REQUIRE(lastOptimizerStep == test::CpuLinearTrainingFixture::PhysicalBatchCount);
    REQUIRE(lastScheduleCount == fixture.optimizerSchedule->callCount());
    REQUIRE(fixture.optimizerSchedule->callCount() == test::CpuLinearTrainingFixture::PhysicalBatchCount);

    REQUIRE(trainResult.ndata() == test::CpuLinearTrainingFixture::Ndata);
    REQUIRE(trainedWeight > initialWeight);
    REQUIRE(trainedWeight < 2.0f);

    double uncertainty = 0.0;
    const double loss = trainResult.loss(&uncertainty);

    REQUIRE(loss > 0.0);
    REQUIRE(job::core::isSafeFinite(static_cast<float>(loss)));
}

TEST_CASE("Managed optimization epoch separates training and validation batches",
          "[ggml][opt][training][usage][epoch][validation]")
{
    test::CpuLinearTrainingFixture fixture{
        true, JobGgmlOptOptimizerType::Sgd, 1, 1.0e-2f
    };

    constexpr std::int64_t idataSplit = 4;

    JobGgmlOptResult trainResult;
    JobGgmlOptResult validationResult;

    std::int64_t trainingCallbackCount   = 0;
    std::int64_t validationCallbackCount = 0;
    std::int64_t stepCallbackCount       = 0;

    JobGgmlOpt::epoch(*fixture.optContext,
                      fixture.dataset,
                      &trainResult,
                      &validationResult,
                      idataSplit,
                      [&trainingCallbackCount](const JobGgmlOptEpochProgress &progress) {
                          REQUIRE(progress.isValid());
                          REQUIRE(progress.isTraining());
                          REQUIRE_FALSE(progress.isValidation());
                          REQUIRE(progress.ibatchMax() == 2);

                          ++trainingCallbackCount;
                      },
                      [&validationCallbackCount](const JobGgmlOptEpochProgress &progress) {
                          REQUIRE(progress.isValid());
                          REQUIRE_FALSE(progress.isTraining());
                          REQUIRE(progress.isValidation());
                          REQUIRE(progress.ibatchMax() == 2);

                          ++validationCallbackCount;
                      },
                      [&stepCallbackCount](const JobGgmlOptStepInfo &step) {
                          REQUIRE(step.isValid());
                          REQUIRE(step.optimizerStep() >= 1);

                          ++stepCallbackCount;
                      });

    REQUIRE(trainingCallbackCount == 2);
    REQUIRE(validationCallbackCount == 2);
    REQUIRE(stepCallbackCount == 2);

    REQUIRE(trainResult.ndata() == 4);
    REQUIRE(validationResult.ndata() == 4);

    REQUIRE(fixture.readWeight() > 0.0f);
}

TEST_CASE("Managed optimization epoch honors logical optimizer accumulation period",
          "[ggml][opt][training][usage][epoch][accumulation]")
{
    constexpr std::int32_t optimizerPeriod = 2;

    test::CpuLinearTrainingFixture fixture{
        true, JobGgmlOptOptimizerType::Sgd, optimizerPeriod, 1.0e-2f
    };

    JobGgmlOptResult trainResult;

    std::int64_t trainingCallbackCount = 0;
    std::int64_t stepCallbackCount     = 0;

    JobGgmlOpt::epoch(*fixture.optContext,
                      fixture.dataset,
                      &trainResult,
                      nullptr,
                      -1,
                      [&trainingCallbackCount](const JobGgmlOptEpochProgress &progress) {
                          REQUIRE(progress.isTraining());
                          ++trainingCallbackCount;
                      },
                      {},
                      [&stepCallbackCount](const JobGgmlOptStepInfo &step) {
                          REQUIRE(step.isValid());
                          ++stepCallbackCount;
                      });

    REQUIRE(trainingCallbackCount == test::CpuLinearTrainingFixture::PhysicalBatchCount);
    REQUIRE(stepCallbackCount == test::CpuLinearTrainingFixture::PhysicalBatchCount / optimizerPeriod);
    REQUIRE(fixture.optimizerSchedule->callCount() == stepCallbackCount);
    REQUIRE(trainResult.ndata() == test::CpuLinearTrainingFixture::Ndata);
}

TEST_CASE("Managed fit learns the scalar relation y equals two x",
          "[ggml][opt][training][usage][fit][sgd]")
{
    test::CpuLinearTrainingFixture fixture;

    constexpr std::int64_t epochCount       = 40;
    constexpr std::int64_t logicalBatchSize = test::CpuLinearTrainingFixture::NdataBatch;

    auto optimizerSchedule = test::CpuLinearTrainingFixture::createSchedule(
        JobGgmlOptOptimizerType::Sgd, 1.0e-2f);

    REQUIRE(optimizerSchedule != nullptr);
    REQUIRE(optimizerSchedule->isValid());

    const float initialWeight = fixture.readWeight();

    std::int64_t trainingCallbackCount = 0;
    std::int64_t stepCallbackCount     = 0;

    std::int64_t lastEpoch         = 0;
    std::int64_t lastOptimizerStep = 0;
    std::int64_t lastCallbackCount = 0;

    double firstObservedLoss = 0.0;
    double finalObservedLoss = 0.0;

    JobGgmlOpt::fit(
        *fixture.schedulerFixture.scheduler(),
        *fixture.computeContext,
        *fixture.inputs,
        *fixture.outputs,
        fixture.dataset,
        JobGgmlOptLossType::MeanSquaredError,
        JobGgmlOptOptimizerType::Sgd,
        optimizerSchedule,
        epochCount,
        logicalBatchSize,
        0.0f,
        [&trainingCallbackCount, &firstObservedLoss, &finalObservedLoss](
            const JobGgmlOptEpochProgress &progress) {
            REQUIRE(progress.isValid());
            REQUIRE(progress.isTraining());

            double uncertainty = 0.0;
            const double loss  = progress.result()->loss(&uncertainty);

            REQUIRE(job::core::isSafeFinite(static_cast<float>(loss)));

            if (trainingCallbackCount == 0) {
                firstObservedLoss = loss;
            }
            finalObservedLoss = loss;

            ++trainingCallbackCount;
        },
        {},
        [&stepCallbackCount, &lastEpoch, &lastOptimizerStep, &lastCallbackCount](
            const JobGgmlOptStepInfo &step) {
            REQUIRE(step.isValid());
            REQUIRE(step.epoch() >= 1);
            REQUIRE(step.optimizerStep() >= 1);
            REQUIRE(step.callbackCount() >= 1);

            ++stepCallbackCount;
            lastEpoch         = step.epoch();
            lastOptimizerStep = step.optimizerStep();
            lastCallbackCount = step.callbackCount();
        },
        true);

    const float finalWeight = fixture.readWeight();
    const std::int64_t expectedPhysicalBatchCount =
        epochCount * test::CpuLinearTrainingFixture::PhysicalBatchCount;

    REQUIRE(trainingCallbackCount == expectedPhysicalBatchCount);
    REQUIRE(stepCallbackCount == expectedPhysicalBatchCount);
    REQUIRE(lastEpoch == epochCount);
    REQUIRE(lastOptimizerStep == expectedPhysicalBatchCount);
    REQUIRE(optimizerSchedule->callCount() == expectedPhysicalBatchCount);
    REQUIRE(lastCallbackCount == optimizerSchedule->callCount());
    REQUIRE(finalObservedLoss < firstObservedLoss);
    REQUIRE(finalWeight > initialWeight);
    REQUIRE(finalWeight == Approx(2.0f).margin(0.15f));
}

TEST_CASE("Managed fit reports validation progress separately",
          "[ggml][opt][training][usage][fit][validation]")
{
    test::CpuLinearTrainingFixture fixture;

    constexpr std::int64_t epochCount       = 4;
    constexpr std::int64_t logicalBatchSize = test::CpuLinearTrainingFixture::NdataBatch;
    constexpr float validationSplit         = 0.5f;

    auto optimizerSchedule = test::CpuLinearTrainingFixture::createSchedule(
        JobGgmlOptOptimizerType::Sgd, 1.0e-2f);

    std::int64_t trainingCallbackCount   = 0;
    std::int64_t validationCallbackCount = 0;
    std::int64_t stepCallbackCount       = 0;

    JobGgmlOpt::fit(
        *fixture.schedulerFixture.scheduler(),
        *fixture.computeContext,
        *fixture.inputs,
        *fixture.outputs,
        fixture.dataset,
        JobGgmlOptLossType::MeanSquaredError,
        JobGgmlOptOptimizerType::Sgd,
        optimizerSchedule,
        epochCount,
        logicalBatchSize,
        validationSplit,
        [&trainingCallbackCount](const JobGgmlOptEpochProgress &progress) {
            REQUIRE(progress.isTraining());
            REQUIRE_FALSE(progress.isValidation());
            ++trainingCallbackCount;
        },
        [&validationCallbackCount](const JobGgmlOptEpochProgress &progress) {
            REQUIRE_FALSE(progress.isTraining());
            REQUIRE(progress.isValidation());
            ++validationCallbackCount;
        },
        [&stepCallbackCount](const JobGgmlOptStepInfo &step) {
            REQUIRE(step.isValid());
            ++stepCallbackCount;
        },
        true);

    constexpr std::int64_t physicalBatchesPerEpoch   = test::CpuLinearTrainingFixture::PhysicalBatchCount;
    constexpr std::int64_t trainingBatchesPerEpoch   = physicalBatchesPerEpoch / 2;
    constexpr std::int64_t validationBatchesPerEpoch = physicalBatchesPerEpoch - trainingBatchesPerEpoch;

    REQUIRE(trainingCallbackCount == epochCount * trainingBatchesPerEpoch);
    REQUIRE(validationCallbackCount == epochCount * validationBatchesPerEpoch);
    REQUIRE(stepCallbackCount == epochCount * trainingBatchesPerEpoch);
    REQUIRE(optimizerSchedule->callCount() == stepCallbackCount);
}

TEST_CASE("Native fit updates the scalar model parameter",
          "[ggml][opt][training][usage][fit][native]")
{
    test::CpuLinearTrainingFixture fixture;

    const float initialWeight = fixture.readWeight();

    JobGgmlOpt::fit(
        *fixture.schedulerFixture.scheduler(),
        *fixture.computeContext,
        *fixture.inputs,
        *fixture.outputs,
        fixture.dataset,
        JobGgmlOptLossType::MeanSquaredError,
        JobGgmlOptOptimizerType::Sgd,
        20,
        test::CpuLinearTrainingFixture::NdataBatch,
        0.0f,
        true);

    const float finalWeight = fixture.readWeight();

    REQUIRE(finalWeight != Approx(initialWeight));
    REQUIRE(finalWeight > initialWeight);
}

TEST_CASE("AdamW managed fit updates the scalar model parameter",
          "[ggml][opt][training][usage][fit][adamw]")
{
    test::CpuLinearTrainingFixture fixture;

    auto optimizerSchedule = test::CpuLinearTrainingFixture::createSchedule(
        JobGgmlOptOptimizerType::AdamW, 5.0e-2f);

    const float initialWeight = fixture.readWeight();
    std::int64_t stepCallbackCount = 0;

    JobGgmlOpt::fit(
        *fixture.schedulerFixture.scheduler(),
        *fixture.computeContext,
        *fixture.inputs,
        *fixture.outputs,
        fixture.dataset,
        JobGgmlOptLossType::MeanSquaredError,
        JobGgmlOptOptimizerType::AdamW,
        optimizerSchedule,
        30,
        test::CpuLinearTrainingFixture::NdataBatch,
        0.0f,
        {},
        {},
        [&stepCallbackCount](const JobGgmlOptStepInfo &step) {
            REQUIRE(step.isValid());
            ++stepCallbackCount;
        },
        true);

    const float finalWeight = fixture.readWeight();

    REQUIRE(stepCallbackCount > 0);
    REQUIRE(optimizerSchedule->callCount() == stepCallbackCount);
    REQUIRE(finalWeight > initialWeight);
    REQUIRE(finalWeight < 3.0f);
}

// ============================================================================
// Block two: edge cases / invariants
// ============================================================================

TEST_CASE("Managed epoch requires a training result when reporting training progress",
          "[ggml][opt][training][edge][epoch][callback]")
{
    test::CpuLinearTrainingFixture fixture{true};

    REQUIRE_THROWS_AS(
        JobGgmlOpt::epoch(
            *fixture.optContext,
            fixture.dataset,
            nullptr,
            nullptr,
            -1,
            [](const JobGgmlOptEpochProgress &) {},
            {},
            {}),
        std::invalid_argument);
}

TEST_CASE("Managed epoch requires a validation result when reporting validation progress",
          "[ggml][opt][training][edge][epoch][callback][validation]")
{
    test::CpuLinearTrainingFixture fixture{true};
    JobGgmlOptResult trainResult;

    REQUIRE_THROWS_AS(
        JobGgmlOpt::epoch(
            *fixture.optContext,
            fixture.dataset,
            &trainResult,
            nullptr,
            4,
            {},
            [](const JobGgmlOptEpochProgress &) {},
            {}),
        std::invalid_argument);
}

TEST_CASE("Optimization epoch rejects a split outside the dataset",
          "[ggml][opt][training][edge][epoch][split]")
{
    test::CpuLinearTrainingFixture fixture{true};
    JobGgmlOptResult trainResult;

    REQUIRE_THROWS_AS(
        JobGgmlOpt::epoch(
            *fixture.optContext,
            fixture.dataset,
            &trainResult,
            nullptr,
            test::CpuLinearTrainingFixture::Ndata + 1),
        std::out_of_range);

    REQUIRE_THROWS_AS(
        JobGgmlOpt::epoch(
            *fixture.optContext,
            fixture.dataset,
            &trainResult,
            nullptr,
            -2),
        std::out_of_range);
}

TEST_CASE("Optimization epoch rejects a split not aligned to a physical batch",
          "[ggml][opt][training][edge][epoch][split][alignment]")
{
    test::CpuLinearTrainingFixture fixture{true};
    JobGgmlOptResult trainResult;

    REQUIRE_THROWS_AS(
        JobGgmlOpt::epoch(
            *fixture.optContext,
            fixture.dataset,
            &trainResult,
            nullptr,
            3),
        std::invalid_argument);
}

TEST_CASE("Optimization fit rejects a non-positive epoch count",
          "[ggml][opt][training][edge][fit][epoch]")
{
    test::CpuLinearTrainingFixture fixture;

    REQUIRE_THROWS_AS(
        JobGgmlOpt::fit(
            *fixture.schedulerFixture.scheduler(),
            *fixture.computeContext,
            *fixture.inputs,
            *fixture.outputs,
            fixture.dataset,
            JobGgmlOptLossType::MeanSquaredError,
            JobGgmlOptOptimizerType::Sgd,
            0,
            test::CpuLinearTrainingFixture::NdataBatch,
            0.0f,
            true),
        std::invalid_argument);
}

TEST_CASE("Optimization fit rejects an invalid logical batch size",
          "[ggml][opt][training][edge][fit][batch]")
{
    test::CpuLinearTrainingFixture fixture;

    REQUIRE_THROWS_AS(
        JobGgmlOpt::fit(
            *fixture.schedulerFixture.scheduler(),
            *fixture.computeContext,
            *fixture.inputs,
            *fixture.outputs,
            fixture.dataset,
            JobGgmlOptLossType::MeanSquaredError,
            JobGgmlOptOptimizerType::Sgd,
            1,
            0,
            0.0f,
            true),
        std::invalid_argument);

    REQUIRE_THROWS_AS(
        JobGgmlOpt::fit(
            *fixture.schedulerFixture.scheduler(),
            *fixture.computeContext,
            *fixture.inputs,
            *fixture.outputs,
            fixture.dataset,
            JobGgmlOptLossType::MeanSquaredError,
            JobGgmlOptOptimizerType::Sgd,
            1,
            3,
            0.0f,
            true),
        std::invalid_argument);
}

TEST_CASE("Optimization fit rejects invalid validation splits",
          "[ggml][opt][training][edge][fit][validation]")
{
    test::CpuLinearTrainingFixture fixture;

    REQUIRE_THROWS_AS(
        JobGgmlOpt::fit(
            *fixture.schedulerFixture.scheduler(),
            *fixture.computeContext,
            *fixture.inputs,
            *fixture.outputs,
            fixture.dataset,
            JobGgmlOptLossType::MeanSquaredError,
            JobGgmlOptOptimizerType::Sgd,
            1,
            test::CpuLinearTrainingFixture::NdataBatch,
            -0.1f,
            true),
        std::invalid_argument);

    REQUIRE_THROWS_AS(
        JobGgmlOpt::fit(
            *fixture.schedulerFixture.scheduler(),
            *fixture.computeContext,
            *fixture.inputs,
            *fixture.outputs,
            fixture.dataset,
            JobGgmlOptLossType::MeanSquaredError,
            JobGgmlOptOptimizerType::Sgd,
            1,
            test::CpuLinearTrainingFixture::NdataBatch,
            1.0f,
            true),
        std::invalid_argument);

    REQUIRE_THROWS_AS(
        JobGgmlOpt::fit(
            *fixture.schedulerFixture.scheduler(),
            *fixture.computeContext,
            *fixture.inputs,
            *fixture.outputs,
            fixture.dataset,
            JobGgmlOptLossType::MeanSquaredError,
            JobGgmlOptOptimizerType::Sgd,
            1,
            test::CpuLinearTrainingFixture::NdataBatch,
            job::core::safeNaN(),
            true),
        std::invalid_argument);
}

TEST_CASE("Native optimization fit requires an optimizer parameter callback",
          "[ggml][opt][training][edge][fit][native_callback]")
{
    test::CpuLinearTrainingFixture fixture;

    REQUIRE_THROWS_AS(
        JobGgmlOpt::fit(
            *fixture.schedulerFixture.scheduler(),
            *fixture.computeContext,
            *fixture.inputs,
            *fixture.outputs,
            fixture.dataset,
            JobGgmlOptLossType::MeanSquaredError,
            JobGgmlOptOptimizerType::Sgd,
            nullptr,
            1,
            test::CpuLinearTrainingFixture::NdataBatch,
            0.0f,
            true),
        std::invalid_argument);
}

TEST_CASE("Epoch progress reaches completion on the final physical batch",
          "[ggml][opt][training][edge][progress][completion]")
{
    test::CpuLinearTrainingFixture fixture{true};
    JobGgmlOptResult trainResult;
    bool observedCompletion = false;

    JobGgmlOpt::epoch(
        *fixture.optContext,
        fixture.dataset,
        &trainResult,
        nullptr,
        -1,
        [&observedCompletion](const JobGgmlOptEpochProgress &progress) {
            if (progress.ibatch() == progress.ibatchMax()) {
                REQUIRE(progress.isComplete());
                REQUIRE(progress.progress() == Approx(1.0));
                observedCompletion = true;
            }
        },
        {});

    REQUIRE(observedCompletion);
}

// ============================================================================
// Block three: benchmarks / stress
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("One complete scalar training epoch performance",
          "[ggml][opt][training][benchmark][epoch]")
{
    BENCHMARK("train one four-batch scalar epoch") {
        auto fixture = std::make_unique<test::CpuLinearTrainingFixture>(
            true, JobGgmlOptOptimizerType::Sgd, 1, 1.0e-2f);

        JobGgmlOptResult result;
        JobGgmlOpt::epoch(
            *fixture->optContext,
            fixture->dataset,
            &result,
            nullptr,
            -1);

        return fixture;
    };
}

TEST_CASE("Complete managed scalar fit performance", "[ggml][opt][training][benchmark][fit]")
{
    BENCHMARK("fit scalar linear model for ten epochs") {
        auto fixture = std::make_unique<test::CpuLinearTrainingFixture>();
        auto optimizerSchedule = test::CpuLinearTrainingFixture::createSchedule(JobGgmlOptOptimizerType::Sgd, 1.0e-2f);

        JobGgmlOpt::fit(
            *fixture->schedulerFixture.scheduler(),
            *fixture->computeContext,
            *fixture->inputs,
            *fixture->outputs,
            fixture->dataset,
            JobGgmlOptLossType::MeanSquaredError,
            JobGgmlOptOptimizerType::Sgd,
            optimizerSchedule,
            10,
            test::CpuLinearTrainingFixture::NdataBatch,
            0.0f,
            {},
            {},
            true);

        return fixture;
    };
}

TEST_CASE("Repeated managed epoch callback stress", "[ggml][opt][training][benchmark][stress][callbacks]")
{
    constexpr std::int64_t epochCount = 100;

    test::CpuLinearTrainingFixture fixture{
        true,
        JobGgmlOptOptimizerType::Sgd,
        1,
        1.0e-3f
    };

    {
        JobGgmlOptResult result;

        REQUIRE_NOTHROW(JobGgmlOpt::epoch(
            *fixture.optContext,
            fixture.dataset,
            &result,
            nullptr,
            -1,
            [](const JobGgmlOptEpochProgress &progress) {
                REQUIRE(progress.isValid());
            },
            {},
            [](const JobGgmlOptStepInfo &step) {
                REQUIRE(step.isValid());
            }));
    }

    BENCHMARK("run 100 managed training epochs with callbacks") {
        std::int64_t trainingCallbackCount = 0;
        std::int64_t stepCallbackCount     = 0;

        for (std::int64_t epoch = 0; epoch < epochCount; ++epoch) {
            JobGgmlOptResult result;

            JobGgmlOpt::epoch(
                *fixture.optContext,
                fixture.dataset,
                &result,
                nullptr,
                -1,
                [&trainingCallbackCount](const JobGgmlOptEpochProgress &) {
                    ++trainingCallbackCount;
                },
                {},
                [&stepCallbackCount](const JobGgmlOptStepInfo &) {
                    ++stepCallbackCount;
                });
        }

        return trainingCallbackCount + stepCallbackCount;
    };
}

#endif