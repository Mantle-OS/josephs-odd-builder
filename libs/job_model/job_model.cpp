#include "job_model.h"

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <utility>

#include <job_logger.h>

#include <alloc/job_ggml_tensor_allocator.h>
#include <job_ggml_backend_buffer.h>
#include <job_ggml_backend_buffer_type.h>
#include <job_ggml_init_params.h>
#include <job_ggml_tensor_data.h>
#include <job_ggml_tensor_op.h>
#include <job_gguf.h>

#include "graph/arch/qwen_graph_builder.h"
#include "io/gguf_model_config_reader.h"



namespace job::model {

JobModel::JobModel(ggml::JobGgmlDevice &device,
                   ggml::JobGgmlBackendSched &scheduler,
                   DeviceConfig deviceConfig) :
    m_device{device},
    m_scheduler{scheduler},
    m_deviceConfig{std::move(deviceConfig)}
{
    if (!m_device.isValid())
        throw std::invalid_argument{"JobModel requires a valid GGML device"};

    if (!m_device.hasBackend())
        throw std::invalid_argument{"JobModel requires a device with a valid backend"};

    if (!m_scheduler.isValid())
        throw std::invalid_argument{"JobModel requires a valid GGML scheduler"};

    if (!m_deviceConfig.isValid())
        throw std::invalid_argument{"JobModel requires a valid DeviceConfig"};
}

bool JobModel::isLoaded() const noexcept
{
    return
        m_config.isValid() &&
        m_weights.isLoaded() &&
        m_weightCtx &&
        m_weightCtx->isValid() &&
        m_computeCtx &&
        m_computeCtx->isValid() &&
        m_kvCache &&
        m_graphBuilder;
}

void JobModel::reset() noexcept
{
    //
    // Destroy graph/session objects before the contexts and weights they
    // borrow from.
    //
    m_graphBuilder.reset();
    m_kvCache.reset();

    m_computeCtx.reset();

    m_weights.clear();
    m_weightCtx.reset();

    m_config = ModelConfig{};

    //
    // m_device and m_scheduler are borrowed runtime resources.
    // DeviceConfig is application policy and survives model reset.
    //
}

bool JobModel::load(const std::filesystem::path &ggufPath,
                    uint32_t maxContextLength)
{
    reset();

    if (!loadConfigFromGguf(ggufPath)) {
        reset();
        return false;
    }

    if (!loadWeights(ggufPath)) {
        reset();
        return false;
    }

    if (!createKvCache(maxContextLength)) {
        reset();
        return false;
    }

    if (!createComputeContext()) {
        reset();
        return false;
    }

    if (!createGraphBuilder()) {
        reset();
        return false;
    }

    JOB_LOG_INFO(
        "[JobModel] Loaded '{}' (architecture={}, layers={}, ctx={})",
        m_config.archConfig().modelName(),
        m_config.architectureName(),
        m_config.transformerConfig().blockCount(),
        m_kvCache->maxContextLength());

    return true;
}

bool JobModel::load(const std::filesystem::path &ggufPath,
                    ModelConfig config,
                    uint32_t maxContextLength)
{
    reset();

    if (!std::filesystem::exists(ggufPath)) {
        JOB_LOG_ERROR(
            "[JobModel] GGUF path does not exist: {}",
            ggufPath.string());

        return false;
    }

    if (!config.isValid()) {
        JOB_LOG_ERROR("[JobModel] Provided ModelConfig failed validation");
        return false;
    }

    m_config = std::move(config);

    if (!loadWeights(ggufPath)) {
        reset();
        return false;
    }

    if (!createKvCache(maxContextLength)) {
        reset();
        return false;
    }

    if (!createComputeContext()) {
        reset();
        return false;
    }

    if (!createGraphBuilder()) {
        reset();
        return false;
    }

    JOB_LOG_INFO(
        "[JobModel] Loaded '{}' (architecture={}, layers={}, ctx={})",
        m_config.archConfig().modelName(),
        m_config.architectureName(),
        m_config.transformerConfig().blockCount(),
        m_kvCache->maxContextLength());

    return true;
}

bool JobModel::loadConfigFromGguf(const std::filesystem::path &ggufPath)
{
    if (!std::filesystem::exists(ggufPath)) {
        JOB_LOG_ERROR(
            "[JobModel] GGUF path does not exist: {}",
            ggufPath.string());

        return false;
    }

    GgufModelConfigReader reader;

    if (!reader.read(ggufPath, m_config)) {
        JOB_LOG_ERROR(
            "[JobModel] Failed to read model configuration from GGUF: {}",
            ggufPath.string());

        return false;
    }

    if (!m_config.isValid()) {
        JOB_LOG_ERROR("[JobModel] GGUF produced an invalid ModelConfig");

        return false;
    }

    return true;
}

bool JobModel::loadWeights(const std::filesystem::path &ggufPath)
{
    ggml::JobGgmlContext::UPtr weightCtx;

    ggml::JobGguf gguf{&weightCtx};

    //
    // DeviceConfig::noAlloc() describes the GGUF tensor-data allocation
    // preference. mmap/mlock have no direct JobGguf wrapper hook yet, so
    // they are intentionally not faked here.
    //
    if (auto *params = gguf.initParams()) {
        params->setNoAlloc(m_deviceConfig.noAlloc());
        params->setCreateContext(true);
        params->setContextOutput(&weightCtx);
    }

    if (!gguf.open(ggufPath)) {
        JOB_LOG_ERROR(
            "[JobModel] Failed to open GGUF '{}': {}",
            ggufPath.string(),
            gguf.errorString());

        return false;
    }

    if (!weightCtx || !weightCtx->isValid()) {
        JOB_LOG_ERROR("[JobModel] GGUF failed to produce a valid weight context");

        return false;
    }

    //
    // If noAlloc is requested, this model layer currently has no general
    // weight-placement loader capable of materializing GGUF tensor bytes
    // into the resolved backend buffer. Refuse to pretend the metadata-only
    // tensors are loaded weights.
    //
    if (m_deviceConfig.noAlloc()) {
        JOB_LOG_ERROR(
            "[JobModel] DeviceConfig::noAlloc() requires backend-directed "
            "weight materialization, which is not implemented yet"
            );

        return false;
    }

    if (!m_weights.loadFromContext(*weightCtx, m_config)) {
        JOB_LOG_ERROR(
            "[JobModel] Failed to bind {} model layers from GGUF context",
            m_config.transformerConfig().blockCount());

        return false;
    }

    m_weightCtx = std::move(weightCtx);

    return true;
}

bool JobModel::createKvCache(uint32_t maxContextLength)
{
    const auto *bufferType = m_device.bufferType();

    if (!bufferType || !bufferType->isValid()) {
        JOB_LOG_ERROR (
            "[JobModel] Resolved device does not expose a valid buffer type"
            );

        return false;
    }

    const uint32_t contextLength = maxContextLength > 0 ? maxContextLength : m_config.transformerConfig().contextLength();

    if (contextLength == 0) {
        JOB_LOG_ERROR("[JobModel] Cannot create KV cache with zero context length");
        return false;
    }

    try {
        m_kvCache = KvCache::createUniq(
            m_config,
            *bufferType,
            contextLength,
            ggml::JobGgmlType::F16);
    }
    catch (const std::exception &error) {
        JOB_LOG_ERROR(
            "[JobModel] Failed to create KV cache: {}",
            error.what());

        return false;
    }

    return m_kvCache != nullptr;
}

bool JobModel::createComputeContext()
{
    //
    // Compute tensors are graph metadata. Their backing storage is allocated
    // through the scheduler / explicit backend buffers, not by ggml_context.
    //
    // The scheduler's graph size is already the runtime's configured graph
    // capacity, so use that as the initial metadata sizing signal instead of
    // the old:
    //
    //   4,000,000 tensors + GGML_DEFAULT_GRAPH_SIZE + 1 GiB padding
    //
    // If this eventually needs a separately configurable metadata budget,
    // that becomes an explicit DeviceConfig field.
    //
    const std::size_t graphSize =
        std::max<std::size_t>(m_scheduler.graphSize(), 1);

    const std::size_t tensorCount =
        graphSize * 4;

    auto initParams = ggml::JobGgmlInitParams::createUniqMetadataFor(tensorCount,
                                                                     graphSize,
                                                                     false);

    if (!initParams) {
        JOB_LOG_ERROR("[JobModel] Failed to create compute context init parameters");

        return false;
    }

    m_computeCtx = ggml::JobGgmlContext::createUniq(*initParams);

    if (!m_computeCtx || !m_computeCtx->isValid()) {
        JOB_LOG_ERROR("[JobModel] Failed to create compute metadata context");
        m_computeCtx.reset();
        return false;
    }

    return true;
}

bool JobModel::createGraphBuilder()
{
    if (!m_kvCache) {
        JOB_LOG_ERROR("[JobModel] Cannot create graph builder without a KV cache");
        return false;
    }

    try {
        switch (m_config.archConfig().arch()) {
        case ModelArchitecture::Qwen3:
            m_graphBuilder = QwenGraphBuilder::createUniq(m_config, m_weights, *m_kvCache);
            break;

        default:
            JOB_LOG_ERROR("[JobModel] Unsupported graph architecture '{}'",
                modelArchitectureToString(m_config.archConfig().arch()));

            return false;
        }
    }
    catch (const std::exception &error) {
        JOB_LOG_ERROR(
            "[JobModel] Failed to create architecture graph builder: {}",
            error.what());

        m_graphBuilder.reset();
        return false;
    }

    return m_graphBuilder != nullptr;
}

ggml::JobGgmlTensor::UPtr JobModel::createInputTensor(
    std::span<const int32_t> tokens,
    ggml::JobGgmlBackendBuffer::Ptr &buffer)
{
    if (!m_computeCtx || !m_computeCtx->isValid())
        throw std::runtime_error{"JobModel requires a valid compute context"};

    if (tokens.empty())
        throw std::invalid_argument{"JobModel input token span cannot be empty"};

    auto *bufferType = m_device.bufferType();

    if (!bufferType || !bufferType->isValid())
        throw std::runtime_error{"JobModel device does not expose a valid buffer type"};

    auto tensor =
        m_computeCtx->newTensor1d(
            ggml::JobGgmlType::I32,
            static_cast<int64_t>(tokens.size()));

    if (!tensor || !tensor->isValid())
        throw std::runtime_error{"JobModel failed to create input token tensor"};

    const std::size_t allocationSize = ggml::JobGgmlTensorAllocator::requiredBufferSize(*bufferType, *tensor);
    auto uniqueBuffer = bufferType->allocateBuffer(allocationSize);
    if (!uniqueBuffer || !uniqueBuffer->isValid())
        throw std::runtime_error{"JobModel failed to allocate input token buffer"};

    buffer = ggml::JobGgmlBackendBuffer::Ptr{
        std::move(uniqueBuffer)
    };

    ggml::JobGgmlTensorAllocator allocator{buffer};

    if (!allocator.isValid())
        throw std::runtime_error{"JobModel failed to create input tensor allocator"};

    if (allocator.allocate(*tensor) != ggml::JobGgmlStatus::Success)
        throw std::runtime_error{"JobModel failed to allocate input token tensor"};

    ggml::JobGgmlTensorData data{tensor->tensor()};

    //
    // Direct element writes are only valid for host-accessible buffers.
    // Device-local token input needs the backend tensor-set/copy abstraction,
    // which should live in job_ggml rather than being recreated here.
    //
    if (!data.isHostAccessible()) {
        throw std::runtime_error{
            "JobModel input token buffer is not host accessible; "
            "backend tensor upload is not implemented yet"
        };
    }

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        data.setValueI32(
            static_cast<int64_t>(i),
            tokens[i]);
    }

    return tensor;
}

int32_t JobModel::sampleLastToken(ggml::JobGgmlCGraph &graph,
                                  Sampler &sampler,
                                  std::span<const int32_t> contextTokens)
{
    auto logits = graph.tensor("logits");

    if (!logits || !logits->isValid()) {
        JOB_LOG_ERROR("[JobModel] Graph does not contain valid logits");

        return -1;
    }

    const int64_t vocabSize = logits->extent(0);
    const int64_t sequenceLength = logits->extent(1);

    if (vocabSize <= 0 || sequenceLength <= 0) {
        JOB_LOG_ERROR("[JobModel] Logits tensor has invalid dimensions");

        return -1;
    }

    //
    // Sampler currently requires host-accessible logits. Do not hide a
    // device->host transfer inside JobModel; that belongs in the ggml/runtime
    // transfer layer once the abstraction exists.
    //
    ggml::JobGgmlTensorData logitsData{logits->tensor()};

    if (!logitsData.isHostAccessible()) {
        JOB_LOG_ERROR(
            "[JobModel] Sampler requires host-accessible logits; "
            "device-to-host logits transfer is not implemented yet"
            );

        return -1;
    }

    const std::size_t rowBytes = logitsData.rowSize();

    const std::size_t offset = static_cast<std::size_t>(sequenceLength - 1) * rowBytes;

    auto lastLogits =
        ggml::JobGgmlTensorOp::createUniq(
            const_cast<struct ggml_tensor *>(logits->tensor()),
            m_computeCtx.get())
            ->view1d(vocabSize, offset);

    if (!lastLogits || !lastLogits->isValid()) {
        JOB_LOG_ERROR("[JobModel] Failed to create final-token logits view");

        return -1;
    }

    return sampler.sample(*lastLogits, contextTokens);
}

std::vector<int32_t> JobModel::generate(
    std::span<const int32_t> promptTokens,
    int32_t maxNewTokens,
    const SamplerConfig &samplerConfig)
{
    if (!isLoaded()) {
        JOB_LOG_ERROR("[JobModel] Cannot generate: model is not loaded");

        return {};
    }

    if (promptTokens.empty()) {
        JOB_LOG_ERROR("[JobModel] Cannot generate from an empty prompt");

        return {};
    }

    if (maxNewTokens <= 0)
        return std::vector<int32_t>{promptTokens.begin(), promptTokens.end()};

    m_kvCache->resetPosition();

    std::vector<int32_t> outputTokens{
        promptTokens.begin(),
        promptTokens.end()
    };

    outputTokens.reserve(
        promptTokens.size() +
        static_cast<std::size_t>(maxNewTokens));

    Sampler sampler{samplerConfig};

    uint32_t nPast = 0;

    //
    // ------------------------------------------------------------
    // Prefill
    // ------------------------------------------------------------
    //
    m_computeCtx->reset();

    ggml::JobGgmlBackendBuffer::Ptr inputBuffer;
    ggml::JobGgmlTensor::UPtr inputTensor;

    try {
        inputTensor =
            createInputTensor(
                promptTokens,
                inputBuffer);
    }
    catch (const std::exception &error) {
        JOB_LOG_ERROR(
            "[JobModel] Failed to create prefill input: {}",
            error.what());

        return {};
    }

    auto graph = m_graphBuilder->buildForward(
        *m_computeCtx,
        *inputTensor,
        nPast,
        ggml::JobGgmlType::F32);

    if (!graph || !graph->isValid()) {
        JOB_LOG_ERROR("[JobModel] Failed to build prefill graph");
        return {};
    }

    JOB_LOG_WARN("[JobModel LOOK] scheduler graph size: {}", m_scheduler.graphSize());
    JOB_LOG_WARN("[JobModel LOOK] actual graph nodes: {}", graph->nodeCount());
    JOB_LOG_WARN("[JobModel LOOK] graph capacity: {}", graph->size());

    {
        const auto graphNodes = graph->nodes();

        for (std::size_t index = 0; index < graphNodes.size(); ++index) {
            const auto &node = graphNodes[index];

            if (!node || !node->isValid())
                continue;

            if (!m_device.deviceInterface()->supportsOp(*node)) {
                const auto *operation = node->operation();

                JOB_LOG_WARN(
                    "[JobModel LOOK] unsupported prefill node {}: name='{}', op='{}', type='{}', shape=[{}, {}, {}, {}]",
                    index,
                    node->name(),
                    operation ? operation->operationName() : "unknown",
                    node->typeName(),
                    node->extent(0),
                    node->extent(1),
                    node->extent(2),
                    node->extent(3));
            }
        }
    }

    m_scheduler.splitGraph(*graph);

    if (!m_scheduler.allocateGraph(*graph)) {
        JOB_LOG_ERROR("[JobModel] Failed to allocate prefill graph");

        return {};
    }

    if (m_scheduler.computeGraph(*graph) !=
        ggml::JobGgmlStatus::Success) {

        JOB_LOG_ERROR("[JobModel] Prefill graph execution failed");

        return {};
    }

    m_scheduler.synchronize();

    int32_t nextToken =
        sampleLastToken(
            *graph,
            sampler,
            outputTokens);

    if (nextToken < 0)
        return {};

    outputTokens.push_back(nextToken);

    nPast =
        static_cast<uint32_t>(promptTokens.size());

    m_kvCache->advance(
        static_cast<uint32_t>(promptTokens.size()));

    //
    // ------------------------------------------------------------
    // Decode
    // ------------------------------------------------------------
    //
    for (int32_t step = 1; step < maxNewTokens; ++step) {
        m_computeCtx->reset();
        m_scheduler.reset();

        const int32_t token = nextToken;

        ggml::JobGgmlBackendBuffer::Ptr stepInputBuffer;
        ggml::JobGgmlTensor::UPtr stepInput;

        try {
            stepInput =
                createInputTensor(
                    std::span<const int32_t>{&token, 1},
                    stepInputBuffer);
        }
        catch (const std::exception &error) {
            JOB_LOG_ERROR(
                "[JobModel] Failed to create decode input at step {}: {}",
                step,
                error.what());

            break;
        }

        auto stepGraph =
            m_graphBuilder->buildForward(
                *m_computeCtx,
                *stepInput,
                nPast,
                ggml::JobGgmlType::F32);

        if (!stepGraph || !stepGraph->isValid()) {
            JOB_LOG_ERROR(
                "[JobModel] Failed to build decode graph at step {}",
                step);

            break;
        }

        {
            const auto graphNodes = stepGraph->nodes();

            for (std::size_t index = 0; index < graphNodes.size(); ++index) {
                const auto &node = graphNodes[index];

                if (!node || !node->isValid())
                    continue;

                if (!m_device.deviceInterface()->supportsOp(*node)) {
                    JOB_LOG_WARN(
                        "[JobModel LOOK] device does not support decode graph node {} at step {}: name='{}'",
                        index,
                        step,
                        node->name());
                }
            }
        }

        m_scheduler.splitGraph(*stepGraph);

        if (!m_scheduler.allocateGraph(*stepGraph)) {
            JOB_LOG_ERROR(
                "[JobModel] Failed to allocate decode graph at step {}",
                step);

            break;
        }

        if (m_scheduler.computeGraph(*stepGraph) !=
            ggml::JobGgmlStatus::Success) {

            JOB_LOG_ERROR(
                "[JobModel] Decode graph execution failed at step {}",
                step);

            break;
        }

        m_scheduler.synchronize();

        nextToken =
            sampleLastToken(
                *stepGraph,
                sampler,
                outputTokens);

        if (nextToken < 0)
            break;

        outputTokens.push_back(nextToken);

        ++nPast;
        m_kvCache->advance();
    }

    return outputTokens;
}

} // namespace job::model