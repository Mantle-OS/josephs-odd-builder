#include "job_model.h"

#include <format>
#include <job_logger.h>

#include <job_ggml.h>
#include <job_ggml_backend_buffer_type.h>
#include <job_ggml_tensor_data.h>
#include <job_ggml_tensor_op.h>
#include <graph/compute_graph_builder.h>

namespace job::model {

JobModel::JobModel()
    : m_ggmlSubsystem(ggml::JobGgml::createUniq())
    , m_config()
    , m_weights()
    , m_kvCache()
    , m_weightCtx(nullptr)
    , m_computeCtx(nullptr)
    , m_backend(nullptr)
    , m_scheduler(nullptr)
{
}

JobModel::~JobModel()
{
    reset();
}

JobModel::JobModel(JobModel&&) noexcept = default;
JobModel& JobModel::operator=(JobModel&&) noexcept = default;

bool JobModel::isLoaded() const noexcept
{
    return m_weights.isLoaded() && m_kvCache.isAllocated() && m_scheduler != nullptr;
}

void JobModel::reset()
{
    m_scheduler.reset();
    m_backend.reset();
    m_computeCtx.reset();
    m_weightCtx.reset();
    m_kvCache.clear();
    m_weights.clear();
    m_config = ModelConfig{};
}

bool JobModel::load(const std::filesystem::path& ggufPath, ModelConfig config, uint32_t maxContextLength)
{
    reset();

    if (!std::filesystem::exists(ggufPath)) {
        JOB_LOG_ERROR("[JobModel] GGUF path does not exist: {}", ggufPath.string());
        return false;
    }

    // 1. Enforce explicit preset configuration directly
    m_config = std::move(config);
    if (!m_config.isValid()) {
        JOB_LOG_ERROR("[JobModel] Provided ModelConfig preset failed validation");
        return false;
    }

    // 2. Open GGUF file container with weight context redirection
    ggml::JobGgmlContext::UPtr loadedWeightCtx;
    ggml::JobGguf gguf(&loadedWeightCtx);

    if (!gguf.open(ggufPath)) {
        JOB_LOG_ERROR("[JobModel] Failed to open GGUF file: {}", gguf.errorString());
        return false;
    }

    if (!loadedWeightCtx || !loadedWeightCtx->isValid()) {
        JOB_LOG_ERROR("[JobModel] GGUF reader failed to produce a valid weight context");
        return false;
    }

    m_weightCtx = std::move(loadedWeightCtx);

    // 3. Load weights from GGUF into weight context using preset dimensions
    if (!m_weights.loadFromContext(*m_weightCtx, m_config)) {
        JOB_LOG_ERROR("[JobModel] Failed to bind model weights from context (Layers expected: {})",
                      m_config.m_transformerConfig.m_blockCount);
        reset();
        return false;
    }

    // 4. Initialize KV cache using preset config and safe inference context length
    const uint32_t ctxLen = (maxContextLength > 0) ? maxContextLength : 4096;
    if (!m_kvCache.init(m_config, ctxLen, ggml::JobGgmlType::F16)) {
        JOB_LOG_ERROR("[JobModel] Failed to initialize KV cache for {} layers", m_config.m_transformerConfig.m_blockCount);
        reset();
        return false;
    }

    // 5. Initialize Subsystem, Device Selection, and Scheduler via DeviceManager
    if (!m_ggmlSubsystem || !m_ggmlSubsystem->isValid()) {
        JOB_LOG_ERROR("[JobModel] Failed to initialize JobGgml subsystem");
        reset();
        return false;
    }

    ggml::JobGgmlDeviceManager *manager = m_ggmlSubsystem->deviceManager();
    if (!manager || !manager->isReady()) {
        JOB_LOG_ERROR("[JobModel] JobGgmlDeviceManager is not ready");
        reset();
        return false;
    }

    // Pick primary CUDA device (RTX 5070 Ti) if available, otherwise fallback to CPU
    ggml::JobGgmlDevice *targetDevice = nullptr;
    if (manager->hasCuda() && !manager->cudaDevices().isEmpty()) {
        targetDevice = manager->cuda(0);
        JOB_LOG_INFO("[JobModel] Using discovered CUDA device: {}", targetDevice->uid());
    } else {
        targetDevice = manager->cpu();
        JOB_LOG_INFO("[JobModel] Fallback: Using CPU device");
    }

    if (!targetDevice || !targetDevice->isValid() || !targetDevice->hasBackend()) {
        JOB_LOG_ERROR("[JobModel] Target device or its backend is invalid");
        reset();
        return false;
    }

    m_backend = targetDevice->backend();
    if (!m_backend || !m_backend->isValid()) {
        JOB_LOG_ERROR("[JobModel] Failed to retrieve valid backend from target device");
        reset();
        return false;
    }

    // Let DeviceManager handle scheduler creation + CPU fallback bundling automatically
    m_scheduler = manager->buildScheduler(targetDevice, 131072, false, true);
    if (!m_scheduler || !m_scheduler->isValid()) {
        JOB_LOG_ERROR("[JobModel] Failed to build scheduler from device manager");
        reset();
        return false;
    }

    // Helper lambda to bind all model weights and KV cache tensors to the active backend
    auto bindAllWeightsToBackend = [this]() {
        if (!m_backend || !m_scheduler) return;

        auto bindTensor = [this](const ggml::JobGgmlTensor* t) {
            if (t && t->isValid()) {
                ggml_tensor* raw = const_cast<ggml_tensor*>(t->tensor());
                if (raw) {
                    ggml::JobGgmlTensor nonConst(raw);
                    m_scheduler->setTensorBackend(nonConst, *m_backend);
                }
            }
        };

        // Global weights
        bindTensor(m_weights.tokenEmbd());
        bindTensor(m_weights.outputNorm());
        bindTensor(m_weights.outputNormBias());
        bindTensor(m_weights.output());
        bindTensor(m_weights.positionEmbd());
        bindTensor(m_weights.typeEmbd());

        // Per-layer weights across all transformer blocks
        for (uint32_t i = 0; i < m_config.m_transformerConfig.m_blockCount; ++i) {
            const auto& lw = m_weights.layer(i);
            bindTensor(lw.attnNorm.get());
            bindTensor(lw.attnNormBias.get());
            bindTensor(lw.attnQ.get());
            bindTensor(lw.attnQBias.get());
            bindTensor(lw.attnK.get());
            bindTensor(lw.attnKBias.get());
            bindTensor(lw.attnV.get());
            bindTensor(lw.attnVBias.get());
            bindTensor(lw.attnOut.get());
            bindTensor(lw.attnOutBias.get());
            bindTensor(lw.attnQNorm.get());
            bindTensor(lw.attnKNorm.get());
            bindTensor(lw.postAttnNorm.get());
            bindTensor(lw.ffnNorm.get());
            bindTensor(lw.ffnNormBias.get());
            bindTensor(lw.ffnGate.get());
            bindTensor(lw.ffnGateBias.get());
            bindTensor(lw.ffnUp.get());
            bindTensor(lw.ffnUpBias.get());
            bindTensor(lw.ffnDown.get());
            bindTensor(lw.ffnDownBias.get());
            bindTensor(lw.postFfnNorm.get());

            // CRITICAL: Bind per-layer KV cache tensors so graph splitter resolves them on CUDA0
            const auto& kvEntry = m_kvCache.layer(i);
            if (kvEntry.k) bindTensor(kvEntry.k.get());
            if (kvEntry.v) bindTensor(kvEntry.v.get());
        }
    };

    bindAllWeightsToBackend();

    // 6. Allocate Compute Execution Context with ample node headroom
    const size_t computeCtxSize = ggml::JobGgmlInitParams::estCtxCost(4000000, GGML_DEFAULT_GRAPH_SIZE, false, 1024 * 1024 * 1024);
    ggml::JobGgmlInitParams computeInit(computeCtxSize, nullptr, false);
    m_computeCtx = ggml::JobGgmlContext::createUniq(computeInit);

    JOB_LOG_INFO("[JobModel] Successfully loaded '{}' with static preset (Layers: {}, Ctx: {})",
                 m_config.m_archConfig.m_modelName,
                 m_config.m_transformerConfig.m_blockCount,
                 ctxLen);

    return true;
}

bool JobModel::load(const std::filesystem::path& ggufPath, uint32_t maxContextLength)
{
    return load(ggufPath, m_config, maxContextLength);
}

std::vector<int32_t> JobModel::generate(
    std::span<const int32_t> promptTokens,
    int32_t maxNewTokens,
    const SamplerConfig& samplerConfig)
{
    if (!isLoaded() || promptTokens.empty()) {
        JOB_LOG_ERROR("[JobModel] Cannot generate: model not loaded or empty prompt");
        return {};
    }

    std::vector<int32_t> outputTokens;
    outputTokens.reserve(promptTokens.size() + static_cast<size_t>(maxNewTokens));
    outputTokens.assign(promptTokens.begin(), promptTokens.end());

    Sampler sampler(samplerConfig);
    uint32_t nPast = 0;

    // Pass backend and scheduler pointers into the graph builder
    ComputeGraphBuilder builder(m_config, m_weights, m_kvCache, m_backend.get(), m_scheduler.get());

    auto bindOutputBackend = [this]() {
        auto outputHead = m_weights.output();
        if (!outputHead || !outputHead->isValid()) {
            outputHead = m_weights.tokenEmbd();
        }
        if (outputHead && outputHead->isValid() && m_backend) {
            ggml_tensor* rawTensor = const_cast<ggml_tensor*>(outputHead->tensor());
            if (rawTensor) {
                ggml::JobGgmlTensor nonConstTensor(rawTensor);
                m_scheduler->setTensorBackend(nonConstTensor, *m_backend);
            }
        }
    };

    // ----------------------------------------------------
    // 1. Prefill Phase
    // ----------------------------------------------------
    m_computeCtx->reset();

    auto inputTensor = m_computeCtx->newTensor1d(ggml::JobGgmlType::I32, static_cast<int64_t>(promptTokens.size()));
    ggml::JobGgmlTensorData tokenData(inputTensor->tensor());
    for (size_t i = 0; i < promptTokens.size(); ++i) {
        tokenData.setValueI32(static_cast<int64_t>(i), promptTokens[i]);
    }

    if (m_backend) {
        m_scheduler->setTensorBackend(*inputTensor, *m_backend);
    }

    auto graph = builder.buildForwardGraph(*m_computeCtx, *inputTensor, nPast);
    if (!graph || !graph->isValid()) {
        JOB_LOG_ERROR("[JobModel] Prefill graph build failed");
        return {};
    }

    // Explicitly register the final logits output tensor node with the scheduler backend
    if (m_backend) {
        ggml_tensor* logitsNative = ggml_graph_get_tensor(graph->graph(), "logits");
        if (logitsNative) {
            ggml::JobGgmlTensor logitsTensor(logitsNative);
            m_scheduler->setTensorBackend(logitsTensor, *m_backend);
        }
    }

    bindOutputBackend();
    m_scheduler->splitGraph(*graph);
    if (!m_scheduler->allocateGraph(*graph) || m_scheduler->computeGraph(*graph) != ggml::JobGgmlStatus::Success) {
        JOB_LOG_ERROR("[JobModel] Prefill graph execution failed");
        return {};
    }

    auto logitsTensor = graph->tensor("logits");
    if (!logitsTensor || !logitsTensor->isValid()) return {};

    const int64_t vocabSize = logitsTensor->extent(0);
    const int64_t seqLen = logitsTensor->extent(1);
    const size_t typeSize = ggml_type_size(logitsTensor->ggmlType());
    const size_t rowStride = static_cast<size_t>(vocabSize) * typeSize;
    const size_t offset = (seqLen - 1) * rowStride;

    auto lastTokenLogits = ggml::JobGgmlTensorOp::createUniq(
                               const_cast<struct ggml_tensor*>(logitsTensor->tensor()), m_computeCtx.get())->view1d(vocabSize, offset);

    int32_t nextToken = sampler.sample(*lastTokenLogits, outputTokens);
    outputTokens.push_back(nextToken);
    nPast += static_cast<uint32_t>(promptTokens.size());
    m_kvCache.advance(static_cast<uint32_t>(promptTokens.size()));

    // ----------------------------------------------------
    // 2. Decoding / Generation Loop
    // ----------------------------------------------------
    for (int32_t step = 1; step < maxNewTokens; ++step) {
        m_computeCtx->reset();

        auto stepInput = m_computeCtx->newTensor1d(ggml::JobGgmlType::I32, 1);
        ggml::JobGgmlTensorData stepData(stepInput->tensor());
        stepData.setValueI32(0, nextToken);

        if (m_backend) {
            m_scheduler->setTensorBackend(*stepInput, *m_backend);
        }

        auto stepGraph = builder.buildForwardGraph(*m_computeCtx, *stepInput, nPast);
        if (!stepGraph || !stepGraph->isValid()) {
            JOB_LOG_ERROR("[JobModel] Decoding step {} graph build failed", step);
            break;
        }

        // Explicitly register decoding step logits tensor with the scheduler backend
        if (m_backend) {
            ggml_tensor* stepLogitsNative = ggml_graph_get_tensor(stepGraph->graph(), "logits");
            if (stepLogitsNative) {
                ggml::JobGgmlTensor stepLogitsTensor(stepLogitsNative);
                m_scheduler->setTensorBackend(stepLogitsTensor, *m_backend);
            }
        }

        bindOutputBackend();
        m_scheduler->splitGraph(*stepGraph);
        if (!m_scheduler->allocateGraph(*stepGraph) || m_scheduler->computeGraph(*stepGraph) != ggml::JobGgmlStatus::Success) {
            JOB_LOG_ERROR("[JobModel] Decoding step {} execution failed", step);
            break;
        }

        auto stepLogits = stepGraph->tensor("logits");
        if (!stepLogits || !stepLogits->isValid()) break;

        nextToken = sampler.sample(*stepLogits, outputTokens);
        outputTokens.push_back(nextToken);
        nPast += 1;
        m_kvCache.advance(1);
    }

    return outputTokens;
}

} // namespace job::model