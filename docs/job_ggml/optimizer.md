# Optimizer

The optimizer subsystem provides the JOB GGML interface for training and parameter optimization.

It wraps optimizer configuration, AdamW and SGD parameter sets, datasets, optimizer schedules, training results, step information, epoch progress, optimizer contexts, and the higher-level `JobGgmlOpt` facade used to coordinate optimization workflows.

The subsystem builds on JOB GGML tensors, graphs, backends, and schedulers while exposing training state through C++ objects rather than requiring direct use of the upstream GGML optimizer API.

---

## JobGgmlOpt

`JobGgmlOpt` is the high-level optimization facade for JOB GGML.

The facade exposes both direct wrappers around GGML's native epoch and fit operations and JOB-managed training loops that provide stateful C++ callbacks, optimizer scheduling, explicit progress reporting, and optimizer-step tracking.

`JobGgmlOpt` is a static utility facade and does not own the scheduler, compute context, tensors, dataset, result objects, or callbacks supplied to an optimization operation. Those objects must remain valid for the duration of the call.

### Native Epoch Execution

`epoch()` provides a direct wrapper around GGML's native `ggml_opt_epoch()` operation.

The dataset is divided at `idataSplit`. Datapoints before the split participate in training, while datapoints from the split onward participate in validation without backward or optimizer work.

Training and validation results are optional and are accumulated rather than reset automatically.

Native GGML epoch callbacks may be supplied independently for the training and validation portions of the epoch.

### JOB-Managed Epoch Execution

A second `epoch()` interface implements the epoch loop within JOB rather than delegating the complete loop to `ggml_opt_epoch()`.

This form accepts stateful `std::function` callbacks without requiring global or thread-local callback registries.

Training batches are evaluated with backward allocation enabled. Validation batches are evaluated without backward or optimizer allocation. The training/validation split must align with the physical batch size exposed by the optimization context. :contentReference[oaicite:0]{index=0}

`EpochCallback` is invoked after each completed physical training or validation batch and receives a borrowed `JobGgmlOptEpochProgress` object describing the current subsection.

`StepCallback` is invoked only when an optimizer update has completed. Gradient-accumulation batches that do not reach the configured optimizer period do not produce an optimizer-step callback. :contentReference[oaicite:1]{index=1}

### Native Fit

`fit()` provides direct access to GGML's native `ggml_opt_fit()` workflow.

The caller supplies the backend scheduler, compute context, model input and output tensors, optimization dataset, loss type, optimizer type, epoch count, logical batch size, and validation split.

A native optimizer-parameter callback may be supplied explicitly, or the overload using GGML's default optimizer parameters may be used instead. :contentReference[oaicite:2]{index=2}

### JOB-Managed Fit

The JOB-managed `fit()` implementation builds and owns the temporary optimization state required for a complete training run while preserving caller ownership of the supplied scheduler, context, tensors, and dataset.

The logical batch size represents the number of datapoints contributing to one optimizer update. It must be an integral multiple of the physical batch size represented by the second dimension of the input tensor.

From these values JOB derives the optimizer period:

    optimizer period = logical batch size / physical batch size

This allows multiple physical backward passes to accumulate before one optimizer update is completed. :contentReference[oaicite:3]{index=3}

The JOB-managed fit path constructs `JobGgmlOptParams` and `JobGgmlOptContext`, applies the selected optimizer and optional `JobGgmlOptOptimizerSchedule`, creates independent training and validation result accumulators, and executes the requested number of epochs. :contentReference[oaicite:4]{index=4}

The training portion of the dataset is shuffled between epochs when appropriate. Training and validation results are reset at the beginning of each epoch.

### Progress and Step Callbacks

Training and validation callbacks receive `JobGgmlOptEpochProgress`, which reports physical-batch progress together with the active context, dataset, result object, and subsection start time.

If no progress callback is supplied and `silent` is false, the JOB-managed fit loop uses GGML's built-in stderr progress-bar callback.

Optimizer-step callbacks receive `JobGgmlOptStepInfo`. During a complete fit operation this state tracks the current one-based epoch, the number of completed optimizer updates across the fit, and the current optimizer-schedule callback count. :contentReference[oaicite:5]{index=5}

### Validation

`JobGgmlOpt` validates the complete optimization configuration before entering either the native or JOB-managed fit path.

Validation includes scheduler and context state, tensor and dataset validity, supported loss and optimizer types, epoch count, validation split, physical and logical batch dimensions, dataset divisibility, datapoint and label compatibility, and optimizer-period range. :contentReference[oaicite:6]{index=6}

Epoch execution additionally requires a statically configured optimization context, matching dataset and input dimensions, a valid physical batch size, and a training/validation split aligned to a physical batch boundary. :contentReference[oaicite:7]{index=7}

### Progress Bar

`epochProgressBar()` exposes GGML's built-in epoch progress callback through the JOB interface.

It validates the supplied optimization context, dataset, result, batch counters, and start time before forwarding the progress state to GGML.

## JobGgmlOptEpochProgress

`JobGgmlOptEpochProgress` describes the current progress of one training or validation subsection within a high-level optimization epoch.

The object is owned and updated by `JobGgmlOpt` while executing JOB-managed epoch and fit loops. Progress callbacks receive it as borrowed, read-only state.

The associated `JobGgmlOptContext`, `JobGgmlOptDataset`, and `JobGgmlOptResult` are borrowed. Their owners must keep them alive for the duration of any callback using the progress object.

### Training and Validation State

`train()` reports whether the current subsection is a training pass.

`isTraining()` and `isValidation()` provide convenience queries for the same state.

### Batch Progress

`ibatch()` reports the number of batches evaluated so far in the current subsection.

`ibatchMax()` reports the total number of batches expected in that subsection.

`progress()` returns normalized progress from `0.0` through `1.0`. A subsection with no known maximum reports zero progress, while a completed subsection reports `1.0`.

`isComplete()` reports whether the evaluated batch count has reached the configured batch maximum.

The current batch count may never exceed the maximum batch count.

### Timing

`startTimeUs()` records the time at which evaluation of the current training or validation subsection began, expressed in microseconds.

The value is retained so higher-level callbacks can calculate elapsed time or throughput without requiring the optimizer core to prescribe a presentation format.

### Runtime Objects

`context()` exposes the optimization context performing the work.

`dataset()` exposes the dataset currently being evaluated.

`result()` exposes the result accumulator receiving metrics for the subsection.

These objects remain owned elsewhere and are never released by `JobGgmlOptEpochProgress`.

### Updating Progress

`update()` replaces the complete progress snapshot.

The supplied optimization context, dataset, and result must all be valid. Batch counters and the start time must be non-negative, and `ibatch` may not exceed `ibatchMax`.

This allows one `JobGgmlOptEpochProgress` object to be reused throughout a training or validation subsection while preserving a stable object address for callbacks.



## JobGgmlOptStepInfo

`JobGgmlOptStepInfo` describes the current position of a high-level JOB optimization run.

The object is owned and updated by `JobGgmlOpt` while executing JOB-managed epoch and fit loops. It provides a small, stable progress model that can also be exposed as borrowed read-only state to optimizer schedule callbacks.

`epoch()` reports the current dataset iteration. Epoch values are non-negative and are intended to be one-based while a fit operation is actively progressing.

`optimizerStep()` reports the number of completed optimizer updates. Gradient-accumulation passes that do not perform an optimizer update do not increment this counter.

`callbackCount()` reports how many times the optimizer-parameter schedule has been requested.

All values must remain greater than or equal to zero. The corresponding setters reject negative values.

The increment helpers use saturating arithmetic. When a counter reaches the maximum value representable by `std::int64_t`, it remains at that value and the increment operation returns false rather than overflowing.

`reset()` returns the complete progress state to zero.


## JobGgmlOptContext

`JobGgmlOptContext` owns the native GGML optimization context used to prepare, allocate, evaluate, and inspect an optimization workflow.

The context is created from a valid `JobGgmlOptParams` object. During construction, the complete JOB configuration is converted to native `ggml_opt_params` and passed to GGML. The resulting `ggml_opt_context` remains owned by `JobGgmlOptContext` until destruction.

If an optimizer schedule is present in the supplied parameters, the context retains shared ownership of that schedule for as long as the native optimizer context may reference it through callback user data.

### Static and Dynamic Graphs

An optimization context may use either static graphs supplied through `JobGgmlOptParams` or dynamically prepared graphs.

`usesStaticGraphs()` reports whether the native optimization context was initialized with a complete static graph configuration.

For dynamic graph workflows, `prepareAlloc()` supplies the compute context, forward graph, input tensor, and output tensor to GGML. This prepares the forward graph but does not yet construct all gradient and optimizer graph state.

`allocate()` completes graph allocation and may request backward graph construction. After allocation, gradient and optimizer graph inspection becomes available.

`prepareAlloc()` is not valid for optimizer contexts configured with static graphs.

### Optimization Tensors

The optimizer context exposes the native tensors associated with the training workflow through JOB wrappers.

These include the optimizer inputs, outputs, labels, loss tensor, prediction tensor, and correct-prediction count.

`gradientAccumulator()` exposes the gradient-accumulator tensor associated with a supplied graph node when the optimizer graphs have been prepared and allocated.

The returned `JobGgmlTensor` objects are non-owning wrappers around tensors managed by the native optimization context.

### Optimizer State

`optimizerType()` and `optimizerName()` expose the optimizer currently used by the native context.

`optimizerPeriod()` reports the optimization period captured when the context was created.

The associated `JobGgmlOptOptimizerSchedule` may also be inspected through `optimizerSchedule()`.

### Evaluation

`evaluate()` executes the current optimization evaluation.

A `JobGgmlOptResult` may optionally be supplied to accumulate loss, predictions, accuracy, and related result information. Evaluation may also be performed without a result object when those accumulated statistics are not required.

### Reset

`reset()` resets the native optimization context.

When optimizer state is also reset, the associated optimizer schedule call counter is reset so its schedule begins again from its initial call position.

### Native Access

`context()` exposes the underlying `ggml_opt_context_t` for direct interoperability with the native GGML optimization API.

The native context remains owned by `JobGgmlOptContext` and must not be released independently.

## JobGgmlOptOptimizerSchedule

`JobGgmlOptOptimizerSchedule` provides a C++ callback interface for dynamically updating optimizer parameters during training.

The schedule owns a `JobGgmlOptOptimizerParams` object and invokes a caller-provided `std::function` before each optimizer-parameter request. The callback receives the mutable optimizer parameter set together with the current schedule call count, allowing learning rates, weight decay, momentum values, or other optimizer settings to change over time.

The schedule is designed to plug directly into `JobGgmlOptParams`. `nativeCallback()` exposes the C-compatible callback entry point expected by GGML, while `nativeUserData()` returns the schedule object used by that callback.

### Call Tracking

`callCount()` reports how many times the native optimizer-parameter callback has been invoked.

The count is incremented before the C++ callback is executed and saturates at the maximum value representable by `std::int64_t` rather than overflowing.

`resetCallCount()` resets the schedule counter without replacing the callback or optimizer parameter state.

### Optimizer Parameter State

`optimizerParams()` exposes the mutable `JobGgmlOptOptimizerParams` object used by the schedule.

The schedule also retains the most recent valid native optimizer parameter aggregate. Before each callback invocation, the mutable JOB parameter object is restored from this last known-good state.

If the callback produces a valid new configuration, that configuration becomes the new saved state returned to GGML.

If the callback produces invalid optimizer parameters, the previous valid configuration is returned instead.

### Callback Safety

The native callback boundary is implemented through an internal trampoline.

C++ exceptions are never allowed to cross the GGML C callback ABI. If the user callback throws, the schedule attempts to restore its previous valid JOB-side parameter state and returns the most recent valid native optimizer parameters to GGML.

This gives optimizer schedules fail-safe behavior: a failed schedule update does not leave GGML with a partially modified or invalid optimizer configuration.

## JobGgmlOptResult

`JobGgmlOptResult` owns a native GGML optimization result and provides access to the accumulated evaluation and training statistics exposed by the public `ggml-opt` API.

The native `ggml_opt_result` is created when the wrapper is constructed and released when the wrapper is destroyed. `reset()` clears the accumulated result state while preserving the native result object for reuse.

`ndata()` reports the number of datapoints currently represented by the accumulated result. `isEmpty()` is true when no datapoints have been accumulated.

### Loss

`loss()` returns the accumulated optimization loss.

An optional uncertainty output may be supplied by the caller. JOB always provides valid uncertainty storage to the native GGML call internally, even when the caller does not request it, avoiding reliance on upstream behavior around null uncertainty pointers.

### Predictions

`predictions()` returns the accumulated prediction values as a `std::vector<std::int32_t>`.

The result size is derived from `ndata()`, and the wrapper validates that the native prediction count can be represented by the destination vector before allocating storage.

An empty result returns an empty prediction vector.

### Accuracy

`accuracy()` returns the accumulated prediction accuracy.

As with loss, an optional uncertainty output may be supplied. Invalid result state is represented using `NaN` for accuracy and uncertainty rather than manufacturing a meaningful metric.

### Native Access

`result()` exposes the underlying `ggml_opt_result_t` for direct interoperability with the GGML optimization API.

The returned native object remains owned by `JobGgmlOptResult` and must not be freed independently.

### Upstream Internal State

GGML internally retains additional result state, including the optimization period and whether loss is represented per datapoint.

These values affect native loss aggregation but are not currently exposed through the public `ggml-opt` API, so JOB does not provide accessors for them.


## JobGgmlOptDataset

`JobGgmlOptDataset` owns a native GGML optimization dataset and provides access to its training data, optional labels, sharding, shuffling, and batch extraction facilities.

A dataset is defined by the GGML storage types used for data and labels, the number of elements in each datapoint and label, the total number of datapoints, and the number of datapoints contained in each shard.

The native `ggml_opt_dataset` is owned by `JobGgmlOptDataset` and released when the wrapper is destroyed.

The data and label tensors returned by GGML remain owned by the native dataset. `JobGgmlOptDataset` owns `JobGgmlTensor` wrappers for those tensors but does not independently own their native tensor storage.

### Dataset Shape

`neDatapoint()` reports the number of elements contained in one datapoint.

`neLabel()` reports the number of elements contained in one label. A value of zero creates an unlabeled dataset.

`ndata()` reports the total number of datapoints in the dataset.

`ndataShard()` reports the number of datapoints contained in one shard, while `shardCount()` reports the resulting number of shards.

The total datapoint count must be evenly divisible by the shard size.

The datapoint and label extents must also satisfy the block-size requirements of their respective GGML storage types.

### Data and Labels

`data()` exposes the dataset's data tensor through a borrowed `JobGgmlTensor` wrapper.

When the dataset was created with labels, `labels()` exposes the corresponding label tensor and `hasLabels()` reports true.

The data and label types are available in both JOB and native GGML representations through `dataType()`, `ggmlDataType()`, `labelType()`, and `ggmlLabelType()`.

`nbsData()` reports the number of data bytes contained in one shard.

`nbsLabels()` reports the number of label bytes contained in one shard. It is zero for an unlabeled dataset.

### Shuffling

`shuffle()` randomizes dataset shard ordering using a `JobGgmlOptContext`.

The optional `idata` argument may restrict the shuffle operation to a portion of the dataset. When supplied, the range must align with the configured shard size.

The permutation used by GGML to represent the shuffled shard order is maintained internally by the native dataset and is not currently exposed by the public GGML optimization API.

### Tensor Batch Extraction

`getBatch()` extracts a batch directly into `JobGgmlTensor` objects.

The data batch must be contiguous, use the same GGML type as the dataset, and have its first extent equal to `neDatapoint()`.

For labeled datasets, a label batch must also be supplied. It must be contiguous, use the configured label type, and have its first extent equal to `neLabel()`.

For unlabeled datasets, no label batch may be supplied.

Batch sizes are expressed in complete dataset shards. The supplied tensors must therefore contain an integral number of shards, and the requested batch must remain within the dataset.

### Host Batch Extraction

`getBatchHost()` extracts a batch into caller-provided host memory instead of GGML tensors.

The supplied data byte count must contain an integral number of data shards. A label destination is required exactly when the dataset contains labels.

As with tensor batch extraction, the requested batch must remain within the available shard range.

### Native Access

`dataset()` exposes the underlying `ggml_opt_dataset_t` when direct interoperability with the GGML optimization API is required.

The returned native dataset is borrowed and remains owned by `JobGgmlOptDataset`.

GGML also maintains an internal context, backend buffer, and shard permutation for each optimization dataset. These objects are not currently exposed through the public GGML optimization API and therefore do not have JOB wrappers.




## JobGgmlOptParams

`JobGgmlOptParams` represents the complete runtime configuration used to create and execute a GGML optimizer.

The object is constructed around a valid `JobGgmlBackendSched`, which is borrowed for the lifetime of the optimizer configuration. The scheduler supplies the execution environment used by the native `ggml_opt_params` structure.

The selected loss function is represented by `JobGgmlOptLossType`. Supported loss modes include mean, sum, cross entropy, and mean squared error.

### Static Graph Configuration

GGML may optionally be configured with an existing compute context together with input and output tensors.

`computeContext()`, `inputs()`, and `outputs()` are borrowed objects and form one logical static-graph configuration. Either all three must be supplied or none of them may be supplied.

`usesStaticGraphs()` reports whether the complete static-graph configuration is present.

Partial static-graph configuration is considered invalid.

### Build Configuration

`buildType()` controls which optimizer graph representation GGML should construct.

Supported build modes include forward-only graph construction, gradient graph construction, and full optimization graph construction.

`optPeriod()` controls the optimization period and must remain greater than zero.

### Optimizer Selection

The optimizer may be selected through `JobGgmlOptOptimizerType`. JOB currently exposes the AdamW and SGD optimizer implementations provided by GGML.

The selected optimizer is independent from the optimizer-parameter callback, allowing optimizer configuration to be supplied dynamically while retaining a stable optimizer type.

### Optimizer Scheduling

`JobGgmlOptParams` supports either the native GGML default optimizer-parameter callback or a `JobGgmlOptOptimizerSchedule`.

When a schedule is installed, its native callback and user-data pointer become the optimizer-parameter source used by GGML. Removing the schedule restores the upstream default optimizer callback.

The callback and its user data may also be supplied directly through `setGetOptimizerParams()` when lower-level control is required.

### Native Conversion

`optParams()` converts the complete JOB-side configuration into the native `ggml_opt_params` aggregate expected by GGML.

Borrowed JOB objects are translated to their native scheduler, context, and tensor handles, while JOB enum values are converted to their corresponding GGML enum representations.

`isValid()` verifies the complete configuration, including scheduler validity, optimization period, optimizer-parameter callback, and static-graph consistency.

`reset()` removes any static-graph configuration and optimizer schedule and restores the remaining optimizer settings from GGML's default optimizer parameters while preserving the configured backend scheduler and loss type.

## JobGgmlOptAdamWParams

`JobGgmlOptAdamWParams` represents the AdamW-specific optimizer parameters used by JOB GGML.

The object gives the anonymous AdamW parameter group exposed by upstream GGML a strongly typed C++ representation. It stores the learning rate, first and second moment decay factors, numerical epsilon, and weight-decay coefficient.

`alpha` is the optimizer learning rate and must be finite and greater than zero.

`beta1` and `beta2` control the exponential decay used for the first and second moment estimates. Both values must be finite and fall within `[0, 1)`.

`eps` provides the numerical-stability term used by AdamW and must be finite and greater than zero.

`wd` represents weight decay and must be finite and non-negative.

All setters validate their input before updating the stored value, while `isValid()` verifies the complete parameter set. Construction also rejects invalid combinations before an object can escape.

`reset()` restores the default AdamW configuration:
```cpp
    alpha = 0.001
    beta1 = 0.9
    beta2 = 0.999
    eps   = 1.0e-8
    wd    = 0.0
```

## JobGgmlOptSgdParams

`JobGgmlOptSgdParams` represents the stochastic-gradient-descent-specific optimizer parameters used by JOB GGML.

The object gives the anonymous SGD parameter group exposed by upstream GGML a strongly typed C++ representation. It stores the learning rate and weight-decay coefficient used by the optimizer.

`alpha` is the optimizer learning rate and must be finite and greater than zero.

`wd` represents weight decay and must be finite and greater than or equal to zero.

Construction validates the complete parameter set before an object can escape, while the individual setters validate values before updating the stored state.

`isValid()` verifies that both parameters are finite and satisfy their required ranges.

`reset()` restores the default SGD configuration:
```cpp
    alpha = 1.0e-3
    wd    = 0.0
```


## JobGgmlOptOptimizerParams

`JobGgmlOptOptimizerParams` represents the optimizer-specific parameter groups used by GGML.

Upstream stores AdamW and SGD settings as anonymous nested structures inside `ggml_opt_optimizer_params`. JOB exposes those groups as owned `JobGgmlOptAdamWParams` and `JobGgmlOptSgdParams` objects so each optimizer configuration can be inspected and modified through a strongly typed C++ interface.

The AdamW parameters are available through `adamw()`, while SGD parameters are available through `sgd()`.

The object may be constructed from JOB's default optimizer configuration or from an existing native `ggml_opt_optimizer_params`. `setOptimizerParams()` replaces both optimizer parameter groups from a native aggregate.

`optimizerParams()` converts the current JOB-side state back into the native GGML representation. If the parameter objects are not valid, the default optimizer configuration is returned instead.

`isValid()` requires both owned optimizer parameter groups to exist and individually pass their validation rules.

`reset()` restores both AdamW and SGD parameters to the default GGML-compatible configuration.


