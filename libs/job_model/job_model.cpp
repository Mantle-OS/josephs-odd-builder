#include "job_model.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

// job_core
#include <job_logger.h>

// job_ggml
#include <job_ggml_tensor_allocator.h>
#include <job_ggml_backend_buffer.h>
#include <job_ggml_backend_buffer_type.h>
#include <job_ggml_init_params.h>
#include <job_ggml_tensor_data.h>
#include <job_ggml_tensor_op.h>
#include <job_gguf.h>

#include "arch/qwen_graph_builder.h"
#include "gguf_model_config_reader.h"

namespace job::model {

JobModel::JobModel(ggml::JobGgmlDevice &device, ggml::JobGgmlBackendSched &scheduler) :
    m_device{device},
    m_scheduler{scheduler}
{
    if (!m_device.isValid())
        throw std::invalid_argument{"JobModel requires a valid GGML device"};

    if (!m_device.hasBackend())
        throw std::invalid_argument{"JobModel requires a device with a valid backend"};

    if (!m_scheduler.isValid())
        throw std::invalid_argument{"JobModel requires a valid GGML scheduler"};

    if (!m_config.deviceConfig().isValid())
        throw std::invalid_argument{"JobModel requires a valid DeviceConfig"};
}

bool JobModel::isLoaded() const noexcept
{
    return m_config.isValid() &&
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
    // Destroy graph/session objects before the contexts and weights they
    // borrow from.
    m_graphBuilder.reset();
    m_kvCache.reset();

    m_computeCtx.reset();

    m_weights.clear();
    m_weightCtx.reset();
    m_weightBuffer.reset();

    m_config = ModelConfig{};

    // m_device and m_scheduler are borrowed runtime resources.
    // DeviceConfig is application policy and survives model reset.
}

bool JobModel::load(const std::filesystem::path &ggufPath,
                    uint32_t maxContextLength)
{
    reset();

    if (!loadConfigFromGguf(ggufPath)) {
        reset();
        return false;
    }

    //

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

    JOB_LOG_INFO("[JobModel] Loaded '{}' (architecture={}, layers={}, ctx={})",
                 m_config.archConfig().modelName(),
                 m_config.architectureName(),
                 m_config.transformerConfig().blockCount(),
                 m_kvCache->maxContextLength());
    return true;
}

bool JobModel::load(const std::filesystem::path &ggufPath, ModelConfig config, uint32_t maxContextLength)
{
    reset();

    if (!std::filesystem::exists(ggufPath)) {
        JOB_LOG_ERROR("[JobModel] GGUF path does not exist: {}", ggufPath.string());
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

    JOB_LOG_INFO("[JobModel] Loaded '{}' (architecture={}, layers={}, ctx={})",
                 m_config.archConfig().modelName(),
                 m_config.architectureName(),
                 m_config.transformerConfig().blockCount(),
                 m_kvCache->maxContextLength());

    return true;
}

bool JobModel::loadConfigFromGguf(const std::filesystem::path &ggufPath)
{
    if (!std::filesystem::exists(ggufPath)) {
        JOB_LOG_ERROR("[JobModel] GGUF path does not exist: {}", ggufPath.string());
        return false;
    }

    GgufModelConfigReader reader;
    if (!reader.read(ggufPath, m_config)) {
        JOB_LOG_ERROR("[JobModel] Failed to read model configuration from GGUF: {}",
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

    if (auto *params = gguf.initParams()) {
        params->setNoAlloc(m_config.deviceConfig().noAlloc());
        params->setCreateContext(true);
        params->setContextOutput(&weightCtx);
    }

    if (!gguf.open(ggufPath)) {
        JOB_LOG_ERROR("[JobModel] Failed to open GGUF '{}': {}", ggufPath.string(), gguf.errorString());
        return false;
    }

    if (!weightCtx || !weightCtx->isValid()) {
        JOB_LOG_ERROR("[JobModel] GGUF failed to produce a valid weight context");
        return false;
    }

    const auto *ggufCtx = gguf.context();

    if (!ggufCtx || !ggufCtx->isValid()) {
        JOB_LOG_ERROR("[JobModel] Invalid GGUF metadata context");
        return false;
    }

    const std::int64_t ggufTensorCount = ggufCtx->tensorCount();

    if (ggufTensorCount <= 0) {
        JOB_LOG_ERROR("[JobModel] GGUF contains no tensors");
        return false;
    }

    // Check each GGUF tensor against its corresponding GGML tensor.
    // Do not use firstTensor()/nextTensor() for this measurement.
    std::size_t ggufBytes = 0;
    std::size_t contextBytes = 0;
    std::size_t lastTensorEnd = 0;
    std::size_t metadataMismatches = 0;

    for (std::int64_t i = 0; i < ggufTensorCount; ++i) {
        const auto name = ggufCtx->tensorName(i);
        const auto fileBytes = ggufCtx->tensorSize(i);
        auto tensor = weightCtx->tensor(name);

        if (!tensor || !tensor->isValid()) {
            JOB_LOG_ERROR("[GGUF] Missing context tensor '{}' at index {}", name, i);
            return false;
        }

        const auto memoryBytes = tensor->byteCount();

        if (fileBytes > std::numeric_limits<std::size_t>::max() - ggufBytes ||
            memoryBytes > std::numeric_limits<std::size_t>::max() - contextBytes) {
            JOB_LOG_ERROR("[GGUF] Tensor size accumulation overflow");
            return false;
        }

        ggufBytes += fileBytes;
        contextBytes += memoryBytes;

        const auto offset = ggufCtx->tensorOffset(i);

        if (offset > std::numeric_limits<std::size_t>::max() - fileBytes) {
            JOB_LOG_ERROR("[GGUF] Invalid tensor offset for '{}'", name);
            return false;
        }

        lastTensorEnd = std::max(lastTensorEnd,
                                 static_cast<std::size_t>(offset) + fileBytes);

        if (fileBytes != memoryBytes || tensor->type() != ggufCtx->tensorType(i)) {
            if (metadataMismatches < 10) {
                JOB_LOG_WARN("[GGUF] MISMATCH {} type={} file={} context={}",
                             name, tensor->typeName(), fileBytes, memoryBytes);
            }
            ++metadataMismatches;
        }
    }

    JOB_LOG_INFO("[GGUF] Tensor count:    {}", ggufTensorCount);
    JOB_LOG_INFO("[GGUF] File size:       {}", std::filesystem::file_size(ggufPath));
    JOB_LOG_INFO("[GGUF] Data offset:     {}", ggufCtx->dataOffset());
    JOB_LOG_INFO("[GGUF] Tensor bytes:    {}", ggufBytes);
    JOB_LOG_INFO("[GGUF] Context bytes:   {}", contextBytes);
    JOB_LOG_INFO("[GGUF] Last tensor end: {}", lastTensorEnd);
    JOB_LOG_INFO("[GGUF] Mismatches:      {}", metadataMismatches);

    if (metadataMismatches != 0) {
        JOB_LOG_ERROR("[JobModel] GGUF and GGML tensor metadata disagree");
        return false;
    }

    if (m_config.deviceConfig().noAlloc()) {
        JOB_LOG_ERROR("[JobModel] DeviceConfig::noAlloc() requires backend-directed "
                      "weight materialization, which is not implemented yet");
        return false;
    }

    if (!m_weights.loadFromContext(*weightCtx, m_config)) {
        JOB_LOG_ERROR("[JobModel] Failed to bind {} model layers from GGUF context",
                      m_config.transformerConfig().blockCount());
        return false;
    }

    if (m_device.impl() == ggml::JobGgmlDeviceImpl::Cuda) {
        auto *bufferType = m_device.bufferType();
        auto backend = m_device.backend();

        if (!bufferType || !bufferType->isValid() || !backend || !backend->isValid()) {
            JOB_LOG_ERROR("[JobModel] No valid backend for weight materialization");
            return false;
        }

        const std::size_t tensorCount = static_cast<std::size_t>(ggufTensorCount);

        // Create metadata for exactly the tensors listed by GGUF.
        auto deviceCtx = ggml::JobGgmlContext::createUniqMetadata(tensorCount, 0, false);

        if (!deviceCtx || !deviceCtx->isValid()) {
            JOB_LOG_ERROR("[JobModel] Failed to create device weight metadata context");
            return false;
        }

        const std::size_t alignment = bufferType->alignment();

        if (alignment == 0) {
            JOB_LOG_ERROR("[JobModel] Invalid weight backend alignment");
            return false;
        }

        std::size_t totalBytes = 0;
        std::size_t sourceBytes = 0;
        std::size_t destinationBytes = 0;
        std::size_t paddedBytes = 0;
        std::size_t backendBytes = 0;
        std::size_t mismatchedTensors = 0;
        std::size_t createdTensors = 0;

        // ------------------------------------------------------------
        // Create device tensor metadata and calculate buffer size.
        // ------------------------------------------------------------
        for (std::int64_t i = 0; i < ggufTensorCount; ++i) {
            const auto name = ggufCtx->tensorName(i);
            const auto fileBytes = ggufCtx->tensorSize(i);

            auto source = weightCtx->tensor(name);

            if (!source || !source->isValid()) {
                JOB_LOG_ERROR("[WEIGHTS] Missing source tensor '{}'", name);
                return false;
            }

            const int64_t shape[4] = {
                source->extent(0),
                source->extent(1),
                source->extent(2),
                source->extent(3)
            };

            auto destination = deviceCtx->newTensor(source->type(), 4, shape);

            if (!destination || !destination->isValid()) {
                JOB_LOG_ERROR("[WEIGHTS] Failed to create destination '{}'", name);
                return false;
            }

            destination->setName(name);
            ++createdTensors;

            const auto srcSize = source->byteCount();
            const auto dstSize = destination->byteCount();
            const auto padSize = destination->paddedByteCount();
            const auto backendSize = bufferType->allocationSize(*destination);

            if (srcSize != fileBytes ||
                srcSize != dstSize ||
                dstSize != backendSize) {
                if (mismatchedTensors < 10) {
                    JOB_LOG_WARN(
                        "[WEIGHTS] {} type={} GGUF={} source={} destination={} padded={} backend={}",
                        name, source->typeName(),
                        fileBytes, srcSize, dstSize, padSize, backendSize);
                }
                ++mismatchedTensors;
            }

            const std::size_t bytes =
                ggml::JobGgmlTensorAllocator::requiredBufferSize(*bufferType, *destination);

            if (bytes == 0 ||
                bytes > std::numeric_limits<std::size_t>::max() - alignment ||
                totalBytes > std::numeric_limits<std::size_t>::max() - (bytes + alignment)) {
                JOB_LOG_ERROR("[JobModel] Weight buffer size overflow for '{}'", name);
                return false;
            }

            totalBytes += bytes + alignment;

            sourceBytes += srcSize;
            destinationBytes += dstSize;
            paddedBytes += padSize;
            backendBytes += backendSize;
        }

        JOB_LOG_INFO("[WEIGHTS] GGUF tensor count:         {}", tensorCount);
        JOB_LOG_INFO("[WEIGHTS] Created tensor count:      {}", createdTensors);
        JOB_LOG_INFO("[WEIGHTS] GGUF tensor bytes:         {}", ggufBytes);
        JOB_LOG_INFO("[WEIGHTS] Source tensor bytes:       {}", sourceBytes);
        JOB_LOG_INFO("[WEIGHTS] Destination tensor bytes:  {}", destinationBytes);
        JOB_LOG_INFO("[WEIGHTS] Padded tensor bytes:       {}", paddedBytes);
        JOB_LOG_INFO("[WEIGHTS] Backend allocation bytes:  {}", backendBytes);
        JOB_LOG_INFO("[WEIGHTS] Reserved buffer bytes:     {}", totalBytes);
        JOB_LOG_INFO("[WEIGHTS] Size mismatches:           {}", mismatchedTensors);

        if (mismatchedTensors != 0 || sourceBytes != ggufBytes ||
            createdTensors != tensorCount) {
            JOB_LOG_ERROR("[JobModel] Device weight metadata does not match GGUF");
            return false;
        }

        // ------------------------------------------------------------
        // Allocate one persistent backend buffer.
        // ------------------------------------------------------------
        auto uniqueBuffer = bufferType->allocateBuffer(totalBytes);

        if (!uniqueBuffer || !uniqueBuffer->isValid()) {
            JOB_LOG_ERROR("[JobModel] Failed to allocate {} bytes for model weights", totalBytes);
            return false;
        }

        auto deviceBuffer = ggml::JobGgmlBackendBuffer::Ptr{std::move(uniqueBuffer)};
        ggml::JobGgmlTensorAllocator allocator{deviceBuffer};

        // ------------------------------------------------------------
        // Materialize the original quantized bytes on the backend.
        // ------------------------------------------------------------
        std::size_t uploadedTensors = 0;

        for (std::int64_t i = 0; i < ggufTensorCount; ++i) {
            const auto name = ggufCtx->tensorName(i);
            const auto fileBytes = ggufCtx->tensorSize(i);

            auto source = weightCtx->tensor(name);
            auto destination = deviceCtx->tensor(name);

            if (!source || !source->isValid() ||
                !destination || !destination->isValid()) {
                JOB_LOG_ERROR("[WEIGHTS] Failed to resolve tensor '{}'", name);
                return false;
            }

            if (!source->tensor()->data) {
                JOB_LOG_ERROR("[WEIGHTS] Source tensor '{}' has no data", name);
                return false;
            }

            if (source->byteCount() != fileBytes ||
                destination->byteCount() != fileBytes) {
                JOB_LOG_ERROR("[WEIGHTS] Invalid upload size for '{}'", name);
                return false;
            }

            if (allocator.allocate(*destination) != ggml::JobGgmlStatus::Success) {
                JOB_LOG_ERROR("[WEIGHTS] Failed to allocate weight '{}'", name);
                return false;
            }

            backend->setTensorAsync(*destination, source->tensor()->data, 0, fileBytes);
            backend->synchronize();

            ++uploadedTensors;

            // Temporary round-trip verification of one quantized tensor.
            if (name == "blk.0.attn_k.weight") {
                std::vector<std::byte> downloaded(fileBytes);

                backend->getTensorAsync(*destination, downloaded.data(), 0, downloaded.size());
                backend->synchronize();

                // const bool matches = std::memcmp(
                //     downloaded.data(),
                //     source->tensor()->data,
                //     downloaded.size()) == 0;
                //
                // JOB_LOG_WARN("[JobModel WEIGHTS] {} bytes={} roundTrip={}",
                //              name, downloaded.size(), matches ? "PASS" : "FAIL");
            }
        }

        JOB_LOG_INFO("[WEIGHTS] Uploaded tensor count:     {}", uploadedTensors);

        // ------------------------------------------------------------
        // Rebind model weights to the device-resident tensor context.
        // ------------------------------------------------------------
        ModelWeights deviceWeights;

        if (!deviceWeights.loadFromContext(*deviceCtx, m_config)) {
            JOB_LOG_ERROR("[JobModel] Failed to bind device-resident model weights");
            return false;
        }

        m_weights = std::move(deviceWeights);
        m_weightBuffer = std::move(deviceBuffer);
        weightCtx = std::move(deviceCtx);

        JOB_LOG_INFO("[JobModel] Materialized {} tensors in {} backend bytes",
                     uploadedTensors, totalBytes);
    }

    m_weightCtx = std::move(weightCtx);
    return true;
}


bool JobModel::createKvCache(uint32_t maxContextLength)
{
    const auto *bufferType = m_device.bufferType();

    if (!bufferType || !bufferType->isValid()) {
        JOB_LOG_ERROR("[JobModel] Resolved device does not expose a valid buffer type");
        return false;
    }

    const uint32_t contextLength = maxContextLength > 0 ?
                                       maxContextLength :
                                       m_config.transformerConfig().contextLength();

    if (contextLength == 0) {
        JOB_LOG_ERROR("[JobModel] Cannot create KV cache with zero context length");
        return false;
    }

    try {
        m_kvCache = KvCache::createUniq(m_config, *bufferType, contextLength, ggml::JobGgmlType::F16);
    } catch (const std::exception &error) {
        JOB_LOG_ERROR("[JobModel] Failed to create KV cache: {}", error.what());
        return false;
    }

    return m_kvCache != nullptr;
}

bool JobModel::createComputeContext()
{
    // This eventually needs a separately configurable metadata budget, that becomes an explicit DeviceConfig field.
    const std::size_t graphSize = std::max<std::size_t>(m_scheduler.graphSize(), 1);
    const std::size_t tensorCount = graphSize * 4;
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
        // Oh hell no this is what we are trying to not do but its good for the first lets get this working
        switch (m_config.archConfig().arch()) {
        case ModelArchitecture::Qwen3:
        case ModelArchitecture::Qwen2:
            m_graphBuilder = QwenGraphBuilder::createUniq(m_config, m_weights, *m_kvCache);
            break;

        default:
            JOB_LOG_ERROR("[JobModel] Unsupported graph architecture '{}'",
                          modelArchitectureToString(m_config.archConfig().arch()));
            return false;
        }
    } catch (const std::exception &error) {
        JOB_LOG_ERROR("[JobModel] Failed to create architecture graph builder: {}", error.what());
        m_graphBuilder.reset();
        return false;
    }

    return m_graphBuilder != nullptr;
}

ggml::JobGgmlTensor::UPtr JobModel::createInputTensor(std::span<const int32_t> tokens,
                                                      ggml::JobGgmlBackendBuffer::Ptr &buffer)
{

    if (!m_computeCtx || !m_computeCtx->isValid())
        throw std::runtime_error{"JobModel requires a valid compute context"};

    if (tokens.empty())
        throw std::invalid_argument{"JobModel input token span cannot be empty"};

    auto *bufferType = m_device.bufferType();
    if (!bufferType || !bufferType->isValid())
        throw std::runtime_error{"JobModel device does not expose a valid buffer type"};

    auto tensor = m_computeCtx->newTensor1d(ggml::JobGgmlType::I32, static_cast<int64_t>(tokens.size()));

    if (!tensor || !tensor->isValid())
        throw std::runtime_error{"JobModel failed to create input token tensor"};

    const std::size_t allocationSize = ggml::JobGgmlTensorAllocator::requiredBufferSize(*bufferType, *tensor);

    auto uniqueBuffer = bufferType->allocateBuffer(allocationSize);
    if (!uniqueBuffer || !uniqueBuffer->isValid())
        throw std::runtime_error{"JobModel failed to allocate input token buffer"};

    buffer = ggml::JobGgmlBackendBuffer::Ptr{std::move(uniqueBuffer)};

    ggml::JobGgmlTensorAllocator allocator{buffer};
    if (!allocator.isValid())
        throw std::runtime_error{"JobModel failed to create input tensor allocator"};

    if (allocator.allocate(*tensor) != ggml::JobGgmlStatus::Success)
        throw std::runtime_error{"JobModel failed to allocate input token tensor"};

    // Device-local token input uses the backend tensor-set/copy abstraction
    // provided by job_ggml instead of accessing tensor storage directly.
    auto backend = m_device.backend();

    if (!backend || !backend->isValid())
        throw std::runtime_error{"JobModel requires a valid input transfer backend"};

    backend->setTensorAsync(*tensor, tokens.data(), 0, tokens.size_bytes());
    backend->synchronize();

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

    if (logits->type() != ggml::JobGgmlType::F32) {
        JOB_LOG_ERROR("[JobModel] Sampler requires F32 logits");
        return -1;
    }

    const std::size_t rowBytes = static_cast<std::size_t>(vocabSize) * sizeof(float);
    const std::size_t offset = static_cast<std::size_t>(sequenceLength - 1) * logits->stride(1);

    if (logits->stride(0) != sizeof(float) ||
        offset > logits->byteCount() ||
        rowBytes > logits->byteCount() - offset) {
        JOB_LOG_ERROR("[JobModel] Logits tensor has an unsupported layout");
        return -1;
    }

    ggml::JobGgmlTensorData logitsData{logits->tensor()};

    if (logitsData.isHostAccessible()) {
        const auto *data = static_cast<const std::byte *>(logits->tensor()->data);
        const auto *lastRow = reinterpret_cast<const float *>(data + offset);

        return sampler.sample(
            std::span<const float>{lastRow, static_cast<std::size_t>(vocabSize)},
            contextTokens);
    }

    auto backend = m_scheduler.tensorBackend(*logits);

    if (!backend || !backend->isValid()) {
        JOB_LOG_ERROR("[JobModel] Failed to resolve logits backend");
        return -1;
    }

    std::vector<float> hostLogits(static_cast<std::size_t>(vocabSize));

    try {
        backend->getTensorAsync(*logits, hostLogits.data(), offset, rowBytes);
        backend->synchronize();
    } catch (const std::exception &error) {
        JOB_LOG_ERROR("[JobModel] Failed to download logits: {}", error.what());
        return -1;
    }

    return sampler.sample(std::span<const float>{hostLogits}, contextTokens);
}

std::vector<int32_t> JobModel::generate(std::span<const int32_t> promptTokens, int32_t maxNewTokens, const SamplerConfig &samplerConfig)
{
    m_inferenceTime.reset();

    if (!isLoaded() || promptTokens.empty()) {
        JOB_LOG_ERROR("[JobModel] Cannot generate: model is not loaded OR from an empty prompt");
        return {};
    }

    if (maxNewTokens <= 0)
        return std::vector<int32_t>{promptTokens.begin(), promptTokens.end()};

    m_kvCache->resetPosition();
    std::vector<int32_t> outputTokens{promptTokens.begin(), promptTokens.end()};
    outputTokens.reserve(promptTokens.size() + static_cast<std::size_t>(maxNewTokens));

    Sampler sampler{samplerConfig};
    uint32_t nPast = 0;

    // ------------------------------------------------------------
    // Prefill
    // ------------------------------------------------------------

    startTime(InferencePhase::Prefill, StageType::Reset);
    m_scheduler.synchronize();
    m_scheduler.reset();
    m_computeCtx->reset();
    stopTime(InferencePhase::Prefill, StageType::Reset);


    startTime(InferencePhase::Prefill, StageType::Input);
    ggml::JobGgmlBackendBuffer::Ptr inputBuffer;
    ggml::JobGgmlTensor::UPtr inputTensor;
    try {
        inputTensor = createInputTensor(promptTokens, inputBuffer);
    } catch (const std::exception &error) {
        stopTime(InferencePhase::Prefill, StageType::Input);
        JOB_LOG_ERROR("[JobModel] Failed to create prefill input: {}", error.what());
        return {};
    }
    stopTime(InferencePhase::Prefill, StageType::Input);

    startTime(InferencePhase::Prefill, StageType::Build);
    auto graph = m_graphBuilder->buildForward(*m_computeCtx,
                                              *inputTensor,
                                              nPast,
                                              ggml::JobGgmlType::F32);
    stopTime(InferencePhase::Prefill, StageType::Build);

    if (!graph || !graph->isValid()) {
        JOB_LOG_ERROR("[JobModel] Failed to build prefill graph");
        return {};
    }

    startTime(InferencePhase::Prefill, StageType::Compute);
    const auto prefillStatus = m_scheduler.computeGraph(*graph);
    if (prefillStatus != ggml::JobGgmlStatus::Success) {
        JOB_LOG_ERROR("[JobModel] Prefill graph execution failed");
        return {};
    }
    stopTime(InferencePhase::Prefill, StageType::Compute);

    startTime(InferencePhase::Prefill, StageType::Sync);
    m_scheduler.synchronize();
    stopTime(InferencePhase::Prefill, StageType::Sync);

    startTime(InferencePhase::Prefill, StageType::Sample);
    int32_t nextToken = sampleLastToken(*graph, sampler, outputTokens);
    stopTime(InferencePhase::Prefill, StageType::Sample);

    if (nextToken < 0)
        return {};

    outputTokens.push_back(nextToken);
    nPast = static_cast<uint32_t>(promptTokens.size());
    m_kvCache->advance(static_cast<uint32_t>(promptTokens.size()));


    // ------------------------------------------------------------
    // Decode
    // ------------------------------------------------------------
    using DecodeClock = std::chrono::steady_clock;

    for (int32_t step = 1; step < maxNewTokens; ++step) {
        startTime(InferencePhase::Decode, StageType::Reset);

        // Release scheduler references before resetting compute metadata.
        m_scheduler.reset();
        m_computeCtx->reset();

        stopTime(InferencePhase::Decode, StageType::Reset);

        const int32_t token = nextToken;
        ggml::JobGgmlBackendBuffer::Ptr stepInputBuffer;
        ggml::JobGgmlTensor::UPtr stepInput;

        startTime(InferencePhase::Decode, StageType::Input);

        try {
            stepInput = createInputTensor(std::span<const int32_t>{&token, 1}, stepInputBuffer);
        } catch (const std::exception &error) {
            stopTime(InferencePhase::Decode, StageType::Input);
            JOB_LOG_ERROR("[JobModel] Failed to create decode input at step {}: {}", step, error.what());
            break;
        }

        stopTime(InferencePhase::Decode, StageType::Input);
        startTime(InferencePhase::Decode, StageType::Build);

        auto stepGraph = m_graphBuilder->buildForward(*m_computeCtx,
                                                      *stepInput,
                                                      nPast,
                                                      ggml::JobGgmlType::F32);

        stopTime(InferencePhase::Decode, StageType::Build);

        if (!stepGraph || !stepGraph->isValid()) {
            JOB_LOG_ERROR("[JobModel] Failed to build decode graph at step {}", step);
            break;
        }

        // --------------------------------------------------------
        // Scheduler allocation / graph splitting
        // --------------------------------------------------------
        startTime(InferencePhase::Decode, StageType::ComputeAlloc);
        const auto allocStart = DecodeClock::now();
        const bool allocated = m_scheduler.allocateGraph(*stepGraph);
        const auto allocEnd = DecodeClock::now();

        if (!allocated) {
            stopTime(InferencePhase::Decode, StageType::ComputeAlloc);
            JOB_LOG_ERROR("[JobModel] Failed to allocate decode graph at step {}", step);
            break;
        }
        stopTime(InferencePhase::Decode, StageType::ComputeAlloc);
        // --------------------------------------------------------
        // Graph execution
        // --------------------------------------------------------
        startTime(InferencePhase::Decode, StageType::Compute);
        const auto executeStart = DecodeClock::now();
        const auto decodeStatus = m_scheduler.computeGraph(*stepGraph);
        const auto executeEnd = DecodeClock::now();
        stopTime(InferencePhase::Decode, StageType::Compute);

        // decodeAllocMs += std::chrono::duration<double, std::milli>(allocEnd - allocStart).count();
        // decodeExecuteMs += std::chrono::duration<double, std::milli>(executeEnd - executeStart).count();
        // ++decodeMeasuredSteps;

        if (decodeStatus != ggml::JobGgmlStatus::Success) {
            JOB_LOG_ERROR("[JobModel] Decode graph execution failed at step {}", step);
            break;
        }

        startTime(InferencePhase::Decode, StageType::Sync);

        m_scheduler.synchronize();

        stopTime(InferencePhase::Decode, StageType::Sync);

        startTime(InferencePhase::Decode, StageType::Sample);

        nextToken = sampleLastToken(*stepGraph, sampler, outputTokens);

        stopTime(InferencePhase::Decode, StageType::Sample);

        if (nextToken < 0)
            break;

        if (isStopToken(nextToken))
            break;

        outputTokens.push_back(nextToken);

        ++nPast;
        m_kvCache->advance();
    }

    // if (decodeMeasuredSteps > 0) {
    //     const double steps = static_cast<double>(decodeMeasuredSteps);

    //     JOB_LOG_INFO("[JobModel] Decode scheduler timing ({} steps):", decodeMeasuredSteps);
    //     JOB_LOG_INFO("  Allocate/split: {:.3f} ms total, {:.3f} ms/token",
    //                  decodeAllocMs, decodeAllocMs / steps);
    //     JOB_LOG_INFO("  Execute:        {:.3f} ms total, {:.3f} ms/token",
    //                  decodeExecuteMs, decodeExecuteMs / steps);
    // }


    return outputTokens;
}


bool job::model::JobModel::isStopToken(int32_t tokenId) const noexcept
{
    if (!m_tokenizer || !m_tokenizer->isReady())
        return false;

    const auto *token = m_tokenizer->token();
    if (!token)
        return false;

    const auto *special = token->specialTokens();
    if (!special)
        return false;

    return tokenId == special->eosId() || tokenId == special->eotId();
}

} // namespace job::model