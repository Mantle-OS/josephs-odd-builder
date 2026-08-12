#pragma once

#include <cstdint>
#include <functional>

#include <ggml-opt.h>

#include "job_ggml_enums.h"
#include "job_ggml_opt_epoch_progress.h"
#include "job_ggml_opt_optimizer_schedule.h"
#include "job_ggml_opt_step_info.h"
#include "jobggml_export.h"

namespace job::ggml {

class JobGgmlBackendSched;
class JobGgmlContext;
class JobGgmlOptContext;
class JobGgmlOptDataset;
class JobGgmlOptResult;
class JobGgmlTensor;

/*
 * High-level optimization facade.
 * JobGgmlOpt exposes both:
 * - one-to-one wrappers around GGML's native epoch and fit functions,
 * - JOB-managed epoch and fit loops supporting stateful std::function
 *   callbacks without global callback registries.
 * The facade does not own the scheduler, contexts, tensors, dataset, results,
 * or callbacks supplied to individual operations. Objects passed by reference
 * must remain valid for the duration of the call.
 */

class JOBGGML_EXPORT JobGgmlOpt final
{
public:
    /*
     * Called after one optimizer update has completed.
     *
     * The supplied JobGgmlOptStepInfo is borrowed and remains valid only for
     * the duration of the callback invocation.
     */
    using StepCallback = std::function<void(const JobGgmlOptStepInfo &step)>;

    /*
     * Called after one training or validation batch evaluation.
     *
     * The supplied JobGgmlOptEpochProgress is borrowed and remains valid only
     * for the duration of the callback invocation.
     */
    using EpochCallback = std::function<void(const JobGgmlOptEpochProgress &progress)>;

    JobGgmlOpt() = delete;
    ~JobGgmlOpt() = delete;

    JobGgmlOpt(const JobGgmlOpt &) = delete;
    JobGgmlOpt &operator=(const JobGgmlOpt &) = delete;
    JobGgmlOpt(JobGgmlOpt &&) = delete;
    JobGgmlOpt &operator=(JobGgmlOpt &&) = delete;

    /*
     * Perform one training and validation epoch using ggml_opt_epoch().
     *
     * Datapoints before idataSplit participate in training. Datapoints from
     * idataSplit onward participate in validation without backward or
     * optimizer work.
     *
     * resultTrain and resultEval are optional. Existing result contents are
     * incremented and are not reset automatically.
     *
     * callbackTrain and callbackEval use the native GGML callback signature.
     */
    static void epoch(JobGgmlOptContext &context,
                      JobGgmlOptDataset &dataset,
                      JobGgmlOptResult *resultTrain,
                      JobGgmlOptResult *resultEval,
                      std::int64_t idataSplit,
                      ggml_opt_epoch_callback callbackTrain = nullptr,
                      ggml_opt_epoch_callback callbackEval  = nullptr
                      );

    // Invoke GGML's built-in stderr progress-bar callback.
    static void epochProgressBar(bool train,
                                 const JobGgmlOptContext &context,
                                 const JobGgmlOptDataset &dataset,
                                 const JobGgmlOptResult &result,
                                 std::int64_t ibatch,
                                 std::int64_t ibatchMax,
                                 std::int64_t startTimeUs
                                 );

    /*
     * Perform one epoch using a JOB-managed loop.
     *
     * Unlike the native callback overload, this overload accepts stateful
     * std::function callbacks without requiring global or thread-local
     * callback storage.
     *
     * callbackTrain is invoked after each training batch.
     * callbackEval is invoked after each validation batch.
     * stepCallback is invoked after each completed optimizer update.
     */
    static void epoch(JobGgmlOptContext &context,
                      JobGgmlOptDataset &dataset,
                      JobGgmlOptResult *resultTrain,
                      JobGgmlOptResult *resultEval,
                      std::int64_t idataSplit,
                      EpochCallback callbackTrain,
                      EpochCallback callbackEval,
                      StepCallback stepCallback
                      );

    static void epoch(JobGgmlOptContext &context,
                      JobGgmlOptDataset &dataset,
                      JobGgmlOptResult *resultTrain,
                      JobGgmlOptResult *resultEval,
                      std::int64_t idataSplit,
                      EpochCallback callbackTrain,
                      EpochCallback callbackEval
                      );

    /*
     * Fit a statically defined model using ggml_opt_fit().
     * nepoch: Number of complete dataset iterations.
     * nbatchLogical:
     *     Number of datapoints contributing to one optimizer update. It must
     *     be a multiple of the physical batch size stored in the second
     *     dimension of inputs and outputs.
     * validationSplit:
     *     Fraction of the dataset reserved for validation. Valid range is
     *     [0.0f, 1.0f).
     * getOptimizerParams:
     *     Native optimizer-parameter callback. GGML supplies a pointer to its
     *     current epoch counter as callback userdata.
     */
    static void fit(JobGgmlBackendSched &backendSched,
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
                    bool silent = false
                    );


     // Native fit using GGML's default optimizer parameters.
    static void fit(JobGgmlBackendSched &backendSched,
                    JobGgmlContext &computeContext,
                    JobGgmlTensor &inputs,
                    JobGgmlTensor &outputs,
                    JobGgmlOptDataset &dataset,
                    JobGgmlOptLossType lossType,
                    JobGgmlOptOptimizerType optimizer,
                    std::int64_t nepoch,
                    std::int64_t nbatchLogical,
                    float validationSplit,
                    bool silent = false
                    );

    /*
     * Fit a model using a JOB-managed training loop.
     *
     * optimizerSchedule is shared with the generated JobGgmlOptContext so its
     * callback userdata remains alive for the entire optimization run.
     *
     * callbackTrain and callbackEval report physical-batch progress.
     * stepCallback reports completed optimizer updates.
     *
     * A null optimizerSchedule selects GGML's default optimizer parameters.
     * Empty callbacks are permitted and are simply not invoked.
     */
    static void fit(JobGgmlBackendSched &backendSched,
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
                    bool silent = false
                    );

    /*
     * JOB-managed fit overload without an optimizer-step callback.
     */
    static void fit(JobGgmlBackendSched &backendSched,
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
                    bool silent = false
                    );

private:
    [[nodiscard]] static constexpr enum ggml_opt_loss_type toGgmlLossType(JobGgmlOptLossType type) noexcept
    {
        return static_cast<enum ggml_opt_loss_type>(type);
    }

    [[nodiscard]] static constexpr enum ggml_opt_optimizer_type toGgmlOptimizerType(JobGgmlOptOptimizerType type) noexcept
    {
        return static_cast<enum ggml_opt_optimizer_type>(type);
    }

    static void validateFitArguments(JobGgmlBackendSched &backendSched,
                                     JobGgmlContext &computeContext,
                                     JobGgmlTensor &inputs,
                                     JobGgmlTensor &outputs,
                                     JobGgmlOptDataset &dataset,
                                     JobGgmlOptLossType lossType,
                                     JobGgmlOptOptimizerType optimizer,
                                     std::int64_t nepoch,
                                     std::int64_t nbatchLogical,
                                     float validationSplit
                                     );

    static void validateEpochArguments(JobGgmlOptContext &context,
                                       JobGgmlOptDataset &dataset,
                                       JobGgmlOptResult *resultTrain,
                                       JobGgmlOptResult *resultEval,
                                       std::int64_t idataSplit
                                       );
};

} // namespace job::ggml