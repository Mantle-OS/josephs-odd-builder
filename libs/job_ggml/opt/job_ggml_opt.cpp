#include "job_ggml_opt.h"

#include <stdexcept>
#include <utility>
#include <limits>

#include <real_type.h>
#ifndef NDEBUG
#include <job_logger.h>
#endif


#include "job_ggml_backend_sched.h"
#include "job_ggml_context.h"
#include "job_ggml_opt_context.h"
#include "job_ggml_opt_dataset.h"
#include "job_ggml_opt_params.h"
#include "job_ggml_opt_result.h"
#include "job_ggml_tensor.h"

namespace job::ggml {

void JobGgmlOpt::epoch(JobGgmlOptContext &context,
                       JobGgmlOptDataset &dataset,
                       JobGgmlOptResult *resultTrain,
                       JobGgmlOptResult *resultEval,
                       std::int64_t idataSplit,
                       ggml_opt_epoch_callback callbackTrain,
                       ggml_opt_epoch_callback callbackEval
                       )
{
    validateEpochArguments(
        context,
        dataset,
        resultTrain,
        resultEval,
        idataSplit
        );

    ggml_opt_epoch(
        context.context(),
        dataset.dataset(),
        resultTrain ? resultTrain->result() : nullptr,
        resultEval ? resultEval->result() : nullptr,
        idataSplit,
        callbackTrain,
        callbackEval
        );
}

void JobGgmlOpt::epochProgressBar(
    bool train,
    const JobGgmlOptContext &context,
    const JobGgmlOptDataset &dataset,
    const JobGgmlOptResult &result,
    std::int64_t ibatch,
    std::int64_t ibatchMax,
    std::int64_t startTimeUs
    )
{
    if (!context.isValid()) {
        throw std::invalid_argument{
            "JobGgmlOpt::epochProgressBar requires a valid optimization context"
        };
    }

    if (!dataset.isValid()) {
        throw std::invalid_argument{
            "JobGgmlOpt::epochProgressBar requires a valid optimization dataset"
        };
    }

    if (!result.isValid()) {
        throw std::invalid_argument{
            "JobGgmlOpt::epochProgressBar requires a valid optimization result"
        };
    }

    if (ibatch < 0) {
        throw std::invalid_argument{
            "JobGgmlOpt::epochProgressBar ibatch must be greater than or equal to zero"
        };
    }

    if (ibatchMax < 0) {
        throw std::invalid_argument{
            "JobGgmlOpt::epochProgressBar ibatchMax must be greater than or equal to zero"
        };
    }

    if (ibatch > ibatchMax) {
        throw std::invalid_argument{
            "JobGgmlOpt::epochProgressBar ibatch cannot exceed ibatchMax"
        };
    }

    if (startTimeUs < 0) {
        throw std::invalid_argument{
            "JobGgmlOpt::epochProgressBar startTimeUs must be greater than or equal to zero"
        };
    }

    ggml_opt_epoch_callback_progress_bar(
        train,
        const_cast<ggml_opt_context_t>(context.context()),
        const_cast<ggml_opt_dataset_t>(dataset.dataset()),
        const_cast<ggml_opt_result_t>(result.result()),
        ibatch,
        ibatchMax,
        startTimeUs
        );
}

void JobGgmlOpt::epoch(JobGgmlOptContext &context,
                       JobGgmlOptDataset &dataset,
                       JobGgmlOptResult *resultTrain,
                       JobGgmlOptResult *resultEval,
                       std::int64_t idataSplit,
                       EpochCallback callbackTrain,
                       EpochCallback callbackEval,
                       StepCallback stepCallback
                       )
{
    validateEpochArguments(context,
                           dataset,
                           resultTrain,
                           resultEval,
                           idataSplit
                           );

    if (callbackTrain && !resultTrain) {
        throw std::invalid_argument{
            "JobGgmlOpt training progress callback requires a training result"
        };
    }

    if (callbackEval && !resultEval) {
        throw std::invalid_argument{
            "JobGgmlOpt validation progress callback requires a validation result"
        };
    }

    auto inputs = context.inputs();
    auto labels = context.labels();

    if (!inputs || !inputs->isValid()) {
        throw std::runtime_error{
            "JobGgmlOpt optimization context does not expose valid input tensors"
        };
    }

    if (dataset.hasLabels()) {
        if (!labels || !labels->isValid()) {
            throw std::runtime_error{
                "JobGgmlOpt labeled dataset requires optimization-context label tensors"
            };
        }
    } else if (labels) {
        throw std::runtime_error{
            "JobGgmlOpt unlabeled dataset unexpectedly has optimization-context label tensors"
        };
    }

    const std::int64_t ndata = dataset.ndata();
    const std::int64_t ndataBatch = inputs->extent(1);

    if (ndataBatch <= 0) {
        throw std::runtime_error{
            "JobGgmlOpt optimization-context input batch size is invalid"
        };
    }

    if (ndata % ndataBatch != 0) {
        throw std::invalid_argument{
            "JobGgmlOpt dataset size must be divisible by the physical batch size"
        };
    }

    const std::int64_t nbatches = ndata / ndataBatch;
    const std::int64_t resolvedDataSplit = idataSplit < 0 ? ndata : idataSplit;
    if (resolvedDataSplit % ndataBatch != 0) {
        throw std::invalid_argument{
            "JobGgmlOpt training and validation split must align to a physical batch boundary"
        };
    }

    const std::int64_t ibatchSplit = resolvedDataSplit / ndataBatch;
    JobGgmlOptStepInfo stepInfo;

    JobGgmlOptEpochProgress::UPtr trainingProgress;
    JobGgmlOptEpochProgress::UPtr validationProgress;

    std::int64_t ibatch = 0;
    std::int64_t startTimeUs = ggml_time_us();

    for (; ibatch < ibatchSplit; ++ibatch) {
        context.allocate(true);

        dataset.getBatch(*inputs,
                         labels ? labels.get() : nullptr,
                         ibatch
                         );

        context.evaluate(resultTrain);

        const std::int64_t evaluatedBatch = ibatch + 1;

        if (callbackTrain) {
            if (!trainingProgress) {
                trainingProgress =
                    JobGgmlOptEpochProgress::createUniq(true,
                                                        &context,
                                                        &dataset,
                                                        resultTrain,
                                                        evaluatedBatch,
                                                        ibatchSplit,
                                                        startTimeUs
                                                        );
            } else {
                trainingProgress->update(true,
                                         &context,
                                         &dataset,
                                         resultTrain,
                                         evaluatedBatch,
                                         ibatchSplit,
                                         startTimeUs
                                         );
            }

            callbackTrain(
                *trainingProgress
                );
        }

        /*
         * ggml_opt_alloc() advances opt_i before selecting the graph. Every
         * optPeriod physical backward batches therefore completes exactly one
         * optimizer update.
         */
        if (evaluatedBatch % context.optimizerPeriod() == 0) {
            if (!stepInfo.incrementOptimizerStep()) {
#ifndef NDEBUG
                JOB_LOG_WARN(
                    "[JobGgmlOpt] Epoch optimizer-step counter reached "
                    "std::int64_t maximum and will remain saturated"
                    );
#endif
            }

            const auto schedule =
                context.optimizerSchedule();

            if (schedule) {
                stepInfo.setCallbackCount(
                    schedule->callCount()
                    );
            }

            if (stepCallback)
                stepCallback(stepInfo);
        }
    }

    startTimeUs = ggml_time_us();

    for (; ibatch < nbatches; ++ibatch) {
        context.allocate(false);

        dataset.getBatch(
            *inputs,
            labels ?
                labels.get() :
                nullptr,
            ibatch
            );

        context.evaluate(resultEval);

        const std::int64_t evaluatedBatch =
            ibatch + 1 -
            ibatchSplit;

        const std::int64_t validationBatchCount =
            nbatches -
            ibatchSplit;

        if (callbackEval) {
            if (!validationProgress) {
                validationProgress =
                    JobGgmlOptEpochProgress::createUniq(
                        false,
                        &context,
                        &dataset,
                        resultEval,
                        evaluatedBatch,
                        validationBatchCount,
                        startTimeUs
                        );
            } else {
                validationProgress->update(
                    false,
                    &context,
                    &dataset,
                    resultEval,
                    evaluatedBatch,
                    validationBatchCount,
                    startTimeUs
                    );
            }

            callbackEval(
                *validationProgress
                );
        }
    }
}

void JobGgmlOpt::epoch(
    JobGgmlOptContext &context,
    JobGgmlOptDataset &dataset,
    JobGgmlOptResult *resultTrain,
    JobGgmlOptResult *resultEval,
    std::int64_t idataSplit,
    EpochCallback callbackTrain,
    EpochCallback callbackEval
    )
{
    epoch(
        context,
        dataset,
        resultTrain,
        resultEval,
        idataSplit,
        std::move(callbackTrain),
        std::move(callbackEval),
        {}
        );
}

void JobGgmlOpt::fit(
    JobGgmlBackendSched &backendSched,
    JobGgmlContext &computeContext,
    JobGgmlTensor &inputs,
    JobGgmlTensor &outputs,
    JobGgmlOptDataset &dataset,
    JobGgmlOptLossType lossType,
    JobGgmlOptOptimizerType optimizer,
    ggml_opt_get_optimizer_params getOptimizerParams,
    std::int64_t nepoch,
    std::int64_t nbatchLogical,
    float validationSplit,
    bool silent
    )
{
    validateFitArguments(
        backendSched,
        computeContext,
        inputs,
        outputs,
        dataset,
        lossType,
        optimizer,
        nepoch,
        nbatchLogical,
        validationSplit
        );

    if (!getOptimizerParams) {
        throw std::invalid_argument{
            "JobGgmlOpt::fit requires a valid optimizer-parameter callback"
        };
    }

    ggml_opt_fit(
        backendSched.scheduler(),
        computeContext.context(),
        inputs.tensor(),
        outputs.tensor(),
        dataset.dataset(),
        toGgmlLossType(lossType),
        toGgmlOptimizerType(optimizer),
        getOptimizerParams,
        nepoch,
        nbatchLogical,
        validationSplit,
        silent
        );
}

void JobGgmlOpt::fit(
    JobGgmlBackendSched &backendSched,
    JobGgmlContext &computeContext,
    JobGgmlTensor &inputs,
    JobGgmlTensor &outputs,
    JobGgmlOptDataset &dataset,
    JobGgmlOptLossType lossType,
    JobGgmlOptOptimizerType optimizer,
    std::int64_t nepoch,
    std::int64_t nbatchLogical,
    float validationSplit,
    bool silent
    )
{
    fit(
        backendSched,
        computeContext,
        inputs,
        outputs,
        dataset,
        lossType,
        optimizer,
        ggml_opt_get_default_optimizer_params,
        nepoch,
        nbatchLogical,
        validationSplit,
        silent
        );
}

void JobGgmlOpt::fit(
    JobGgmlBackendSched &backendSched,
    JobGgmlContext &computeContext,
    JobGgmlTensor &inputs,
    JobGgmlTensor &outputs,
    JobGgmlOptDataset &dataset,
    JobGgmlOptLossType lossType,
    JobGgmlOptOptimizerType optimizer,
    JobGgmlOptOptimizerSchedule::Ptr optimizerSchedule,
    std::int64_t nepoch,
    std::int64_t nbatchLogical,
    float validationSplit,
    EpochCallback callbackTrain,
    EpochCallback callbackEval,
    StepCallback stepCallback,
    bool silent
    )
{
    validateFitArguments(
        backendSched,
        computeContext,
        inputs,
        outputs,
        dataset,
        lossType,
        optimizer,
        nepoch,
        nbatchLogical,
        validationSplit
        );

    if (optimizerSchedule &&
        !optimizerSchedule->isValid()) {
        throw std::invalid_argument{
            "JobGgmlOpt::fit received an invalid optimizer schedule"
        };
    }

    const std::int64_t ndata =
        dataset.ndata();

    const std::int64_t nbatchPhysical =
        inputs.extent(1);

    const std::int64_t optPeriod =
        nbatchLogical /
        nbatchPhysical;

    const std::int64_t nbatchesLogical =
        ndata /
        nbatchLogical;

    const std::int64_t ibatchSplit =
        static_cast<std::int64_t>(
            (1.0f - validationSplit) *
            static_cast<float>(nbatchesLogical)
            ) *
        optPeriod;

    const std::int64_t idataSplit =
        ibatchSplit *
        nbatchPhysical;

    JobGgmlOptParams params{
        &backendSched,
        lossType
    };

    params.setComputeContext(
        &computeContext
        );

    params.setInputs(
        &inputs
        );

    params.setOutputs(
        &outputs
        );

    params.setOptPeriod(
        static_cast<std::int32_t>(
            optPeriod
            )
        );

    params.setOptimizer(
        optimizer
        );

    params.setOptimizerSchedule(
        std::move(optimizerSchedule)
        );

    JobGgmlOptContext context{
        params
    };

    if (nbatchLogical < ndata) {
        dataset.shuffle(
            context,
            -1
            );
    }

    JobGgmlOptResult resultTrain;
    JobGgmlOptResult resultEval;

    JobGgmlOptStepInfo fitStepInfo;

    for (std::int64_t currentEpoch = 1;
         currentEpoch <= nepoch;
         ++currentEpoch) {
        fitStepInfo.setEpoch(
            currentEpoch
            );

        if (nbatchLogical < idataSplit) {
            dataset.shuffle(
                context,
                idataSplit
                );
        }

        resultTrain.reset();
        resultEval.reset();

        EpochCallback resolvedTrainCallback =
            callbackTrain;

        EpochCallback resolvedEvalCallback =
            callbackEval;

        if (!silent &&
            !resolvedTrainCallback) {
            resolvedTrainCallback =
                [](const JobGgmlOptEpochProgress &progress) {
                    JobGgmlOpt::epochProgressBar(
                        progress.isTraining(),
                        *progress.context(),
                        *progress.dataset(),
                        *progress.result(),
                        progress.ibatch(),
                        progress.ibatchMax(),
                        progress.startTimeUs()
                        );
                };
        }

        if (!silent &&
            !resolvedEvalCallback) {
            resolvedEvalCallback =
                [](const JobGgmlOptEpochProgress &progress) {
                    JobGgmlOpt::epochProgressBar(
                        progress.isTraining(),
                        *progress.context(),
                        *progress.dataset(),
                        *progress.result(),
                        progress.ibatch(),
                        progress.ibatchMax(),
                        progress.startTimeUs()
                        );
                };
        }

        StepCallback resolvedStepCallback;

        if (stepCallback) {
            resolvedStepCallback =
                [&fitStepInfo, &stepCallback](
                    const JobGgmlOptStepInfo &epochStep
                    ) {
                    if (!fitStepInfo.incrementOptimizerStep()) {
#ifndef NDEBUG
                        JOB_LOG_WARN(
                            "[JobGgmlOpt] Fit optimizer-step counter reached "
                            "std::int64_t maximum and will remain saturated"
                            );
#endif
                    }

                    fitStepInfo.setCallbackCount(
                        epochStep.callbackCount()
                        );

                    stepCallback(
                        fitStepInfo
                        );
                };
        }

        epoch(
            context,
            dataset,
            &resultTrain,
            &resultEval,
            idataSplit,
            std::move(resolvedTrainCallback),
            std::move(resolvedEvalCallback),
            std::move(resolvedStepCallback)
            );
    }
}

void JobGgmlOpt::fit(
    JobGgmlBackendSched &backendSched,
    JobGgmlContext &computeContext,
    JobGgmlTensor &inputs,
    JobGgmlTensor &outputs,
    JobGgmlOptDataset &dataset,
    JobGgmlOptLossType lossType,
    JobGgmlOptOptimizerType optimizer,
    JobGgmlOptOptimizerSchedule::Ptr optimizerSchedule,
    std::int64_t nepoch,
    std::int64_t nbatchLogical,
    float validationSplit,
    EpochCallback callbackTrain,
    EpochCallback callbackEval,
    bool silent
    )
{
    fit(
        backendSched,
        computeContext,
        inputs,
        outputs,
        dataset,
        lossType,
        optimizer,
        std::move(optimizerSchedule),
        nepoch,
        nbatchLogical,
        validationSplit,
        std::move(callbackTrain),
        std::move(callbackEval),
        {},
        silent
        );
}

void JobGgmlOpt::validateFitArguments(
    JobGgmlBackendSched &backendSched,
    JobGgmlContext &computeContext,
    JobGgmlTensor &inputs,
    JobGgmlTensor &outputs,
    JobGgmlOptDataset &dataset,
    JobGgmlOptLossType lossType,
    JobGgmlOptOptimizerType optimizer,
    std::int64_t nepoch,
    std::int64_t nbatchLogical,
    float validationSplit
    )
{
    if (!backendSched.isValid()) {
        throw std::invalid_argument{
            "JobGgmlOpt::fit requires a valid backend scheduler"
        };
    }

    if (!computeContext.isValid()) {
        throw std::invalid_argument{
            "JobGgmlOpt::fit requires a valid compute context"
        };
    }

    if (!inputs.isValid()) {
        throw std::invalid_argument{
            "JobGgmlOpt::fit requires a valid input tensor"
        };
    }

    if (!outputs.isValid()) {
        throw std::invalid_argument{
            "JobGgmlOpt::fit requires a valid output tensor"
        };
    }

    if (!dataset.isValid()) {
        throw std::invalid_argument{
            "JobGgmlOpt::fit requires a valid dataset"
        };
    }

    switch (toGgmlLossType(lossType)) {
    case GGML_OPT_LOSS_TYPE_MEAN:
    case GGML_OPT_LOSS_TYPE_SUM:
    case GGML_OPT_LOSS_TYPE_CROSS_ENTROPY:
    case GGML_OPT_LOSS_TYPE_MEAN_SQUARED_ERROR:
        break;

    default:
        throw std::invalid_argument{
            "JobGgmlOpt::fit received an invalid loss type"
        };
    }

    switch (toGgmlOptimizerType(optimizer)) {
    case GGML_OPT_OPTIMIZER_TYPE_ADAMW:
    case GGML_OPT_OPTIMIZER_TYPE_SGD:
        break;

    case GGML_OPT_OPTIMIZER_TYPE_COUNT:
    default:
        throw std::invalid_argument{
            "JobGgmlOpt::fit received an invalid optimizer type"
        };
    }

    if (nepoch <= 0) {
        throw std::invalid_argument{
            "JobGgmlOpt::fit nepoch must be greater than zero"
        };
    }

    if (nbatchLogical <= 0) {
        throw std::invalid_argument{
            "JobGgmlOpt::fit nbatchLogical must be greater than zero"
        };
    }

    if (!core::isSafeFinite(validationSplit) || validationSplit < 0.0f || validationSplit >= 1.0f) {
        throw std::invalid_argument{
            "JobGgmlOpt::fit validationSplit must be finite and in the range [0.0, 1.0)"
        };
    }

    if (inputs.rank() < 2 ||
        outputs.rank() < 2) {
        throw std::invalid_argument{
            "JobGgmlOpt::fit inputs and outputs must contain a batch dimension"
        };
    }

    if (inputs.extent(0) !=
        dataset.neDatapoint()) {
        throw std::invalid_argument{
            "JobGgmlOpt::fit input datapoint extent does not match the dataset"
        };
    }

    if (inputs.extent(1) <= 0) {
        throw std::invalid_argument{
            "JobGgmlOpt::fit physical batch size must be greater than zero"
        };
    }

    if (outputs.extent(1) !=
        inputs.extent(1)) {
        throw std::invalid_argument{
            "JobGgmlOpt::fit input and output physical batch sizes must match"
        };
    }

    if (dataset.hasLabels() &&
        outputs.extent(0) != dataset.neLabel()) {
        throw std::invalid_argument{
            "JobGgmlOpt::fit output label extent does not match the dataset"
        };
    }

    if (dataset.ndata() %
            nbatchLogical != 0) {
        throw std::invalid_argument{
            "JobGgmlOpt::fit dataset size must be divisible by nbatchLogical"
        };
    }

    if (nbatchLogical %
            inputs.extent(1) != 0) {
        throw std::invalid_argument{
            "JobGgmlOpt::fit nbatchLogical must be divisible by the physical batch size"
        };
    }

    const std::int64_t optPeriod =
        nbatchLogical /
        inputs.extent(1);

    if (optPeriod >
        std::numeric_limits<std::int32_t>::max()) {
        throw std::overflow_error{
            "JobGgmlOpt::fit optimizer period exceeds int32_t"
        };
    }
}

void JobGgmlOpt::validateEpochArguments(
    JobGgmlOptContext &context,
    JobGgmlOptDataset &dataset,
    JobGgmlOptResult *resultTrain,
    JobGgmlOptResult *resultEval,
    std::int64_t idataSplit
    )
{
    if (!context.isValid()) {
        throw std::invalid_argument{
            "JobGgmlOpt::epoch requires a valid optimization context"
        };
    }

    if (!context.usesStaticGraphs()) {
        throw std::invalid_argument{
            "JobGgmlOpt::epoch requires statically allocated optimization graphs"
        };
    }

    if (!dataset.isValid()) {
        throw std::invalid_argument{
            "JobGgmlOpt::epoch requires a valid dataset"
        };
    }

    if (resultTrain &&
        !resultTrain->isValid()) {
        throw std::invalid_argument{
            "JobGgmlOpt::epoch received an invalid training result"
        };
    }

    if (resultEval &&
        !resultEval->isValid()) {
        throw std::invalid_argument{
            "JobGgmlOpt::epoch received an invalid validation result"
        };
    }

    auto inputs =
        context.inputs();

    if (!inputs ||
        !inputs->isValid()) {
        throw std::runtime_error{
            "JobGgmlOpt::epoch optimization context does not expose valid inputs"
        };
    }

    if (inputs->extent(0) !=
        dataset.neDatapoint()) {
        throw std::invalid_argument{
            "JobGgmlOpt::epoch input datapoint extent does not match the dataset"
        };
    }

    const std::int64_t ndataBatch =
        inputs->extent(1);

    if (ndataBatch <= 0) {
        throw std::invalid_argument{
            "JobGgmlOpt::epoch physical batch size must be greater than zero"
        };
    }

    const std::int64_t ndata =
        dataset.ndata();

    if (ndata % ndataBatch != 0) {
        throw std::invalid_argument{
            "JobGgmlOpt::epoch dataset size must be divisible by the physical batch size"
        };
    }

    if (idataSplit < -1 ||
        idataSplit > ndata) {
        throw std::out_of_range{
            "JobGgmlOpt::epoch idataSplit is outside the dataset"
        };
    }

    if (idataSplit >= 0 &&
        idataSplit % ndataBatch != 0) {
        throw std::invalid_argument{
            "JobGgmlOpt::epoch idataSplit must align to a physical batch boundary"
        };
    }
}

} // namespace job::ggml