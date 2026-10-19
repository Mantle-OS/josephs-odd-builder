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
#ifndef NDEBUG
    if (!m_device.isValid())
        throw std::invalid_argument{"JobModel requires a valid GGML device"};

    if (!m_device.hasBackend())
        throw std::invalid_argument{"JobModel requires a device with a valid backend"};

    if (!m_scheduler.isValid())
        throw std::invalid_argument{"JobModel requires a valid GGML scheduler"};

    if (!m_config.deviceConfig().isValid())
        throw std::invalid_argument{"JobModel requires a valid DeviceConfig"};
#endif
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
#ifndef NDEBUG
    JOB_LOG_INFO("[JobModel] Loaded '{}' (architecture={}, layers={}, ctx={})",
                 m_config.archConfig().modelName(),
                 m_config.architectureName(),
                 m_config.transformerConfig().blockCount(),
                 m_kvCache->maxContextLength());
#endif
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
#ifndef NDEBUG
    JOB_LOG_INFO("[JobModel] Loaded '{}' (architecture={}, layers={}, ctx={})",
                 m_config.archConfig().modelName(),
                 m_config.architectureName(),
                 m_config.transformerConfig().blockCount(),
                 m_kvCache->maxContextLength());
#endif
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
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobModel] Failed to read model configuration from GGUF: {}",
                      ggufPath.string());
#endif
        return false;
    }

    if (!m_config.isValid()) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobModel] GGUF produced an invalid ModelConfig");
#endif
        return false;
    }

    return true;
}

bool JobModel::loadWeights(const std::filesystem::path &ggufPath)
{
    ggml::JobGgmlContext::UPtr weightCtx;
    ggml::JobGguf gguf{&weightCtx};

    // DeviceConfig::noAlloc() describes the GGUF tensor-data allocation
    // preference. mmap/mlock have no direct JobGguf wrapper hook yet, so
    // they are intentionally not faked here.
    if (auto *params = gguf.initParams()) {
        params->setNoAlloc(m_config.deviceConfig().noAlloc());
        params->setCreateContext(true);
        params->setContextOutput(&weightCtx);
    }

    if (!gguf.open(ggufPath)) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobModel] Failed to open GGUF '{}': {}",
                      ggufPath.string(), gguf.errorString());
#endif
        return false;
    }

    if (!weightCtx || !weightCtx->isValid()) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobModel] GGUF failed to produce a valid weight context");
#endif
        return false;
    }

    // If noAlloc is requested, this model layer currently has no general
    // weight-placement loader capable of materializing GGUF tensor bytes into the resolved backend buffer.
    // Refuse to pretend the metadata-only tensors are loaded weights.
    if (m_config.deviceConfig().noAlloc()) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobModel] DeviceConfig::noAlloc() requires backend-directed "
                      "weight materialization, which is not implemented yet");
#endif
        return false;
    }

    if (!m_weights.loadFromContext(*weightCtx, m_config)) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobModel] Failed to bind {} model layers from GGUF context",
                      m_config.transformerConfig().blockCount());
#endif
        return false;
    }

    if (m_device.impl() != ggml::JobGgmlDeviceImpl::Cpu) {
        // Stage GGUF's original quantized bytes in host memory, then create
        // persistent backend-backed tensors for the graph builder to borrow.
        auto *bufferType = m_device.bufferType();
        auto backend = m_device.backend();

        if (!bufferType || !bufferType->isValid() || !backend || !backend->isValid()) {
            JOB_LOG_ERROR("[JobModel] No valid backend for weight materialization");
            return false;
        }

        // A separate metadata-only context avoids replacing GGUF's host data
        // pointers while the source bytes are still needed for the upload.
        std::size_t tensorCount = 0;

        for (auto tensor = weightCtx->firstTensor(); tensor; tensor = weightCtx->nextTensor(*tensor))
            ++tensorCount;

        auto deviceCtx = ggml::JobGgmlContext::createUniqMetadata(tensorCount, 0, false);
        if (!deviceCtx || !deviceCtx->isValid()) {
#ifndef NDEBUG
            JOB_LOG_ERROR("[JobModel] Failed to create device weight metadata context");
#endif
            return false;
        }

        std::size_t totalBytes = 0;
        const std::size_t alignment = bufferType->alignment();

        if (alignment == 0) {
#ifndef NDEBUG
            JOB_LOG_ERROR("[JobModel] Invalid weight backend alignment");
#endif
            return false;
        }

        for (auto source = weightCtx->firstTensor(); source; source = weightCtx->nextTensor(*source)) {
            const int64_t shape[4] = {
                source->extent(0),
                source->extent(1),
                source->extent(2),
                source->extent(3)
            };

            auto destination = deviceCtx->newTensor(source->type(), 4, shape);
            if (!destination || !destination->isValid())
                return false;

            destination->setName(source->name());

            const std::size_t bytes = ggml::JobGgmlTensorAllocator::requiredBufferSize(*bufferType, *destination);

            if (bytes == 0 ||
                bytes > std::numeric_limits<std::size_t>::max() - alignment ||
                totalBytes > std::numeric_limits<std::size_t>::max() - (bytes + alignment)) {
#ifndef NDEBUG
                JOB_LOG_ERROR("[JobModel] Weight buffer size overflow for '{}'", source->name());
#endif
                return false;
            }

            totalBytes += bytes + alignment;
        }

        auto uniqueBuffer = bufferType->allocateBuffer(totalBytes);
        if (!uniqueBuffer || !uniqueBuffer->isValid()) {
#ifndef NDEBUG
            JOB_LOG_ERROR("[JobModel] Failed to allocate {} bytes for model weights", totalBytes);
#endif
            return false;
        }

        auto deviceBuffer = ggml::JobGgmlBackendBuffer::Ptr{std::move(uniqueBuffer)};
        ggml::JobGgmlTensorAllocator allocator{deviceBuffer};

        for (auto source = weightCtx->firstTensor(); source; source = weightCtx->nextTensor(*source)) {
            auto destination = deviceCtx->tensor(source->name());

            if (!destination || !destination->isValid() || !source->tensor()->data ||
                allocator.allocate(*destination) != ggml::JobGgmlStatus::Success) {
#ifndef NDEBUG
                JOB_LOG_ERROR("[JobModel] Failed to allocate weight '{}'", source->name());
#endif
                return false;
            }

            backend->setTensorAsync(*destination, source->tensor()->data, 0, source->byteCount());
            backend->synchronize();

            // Temporary verification of original GGUF quantized weight bytes.
            if (source->name() == "blk.0.attn_k.weight") {
                std::vector<std::byte> downloaded(source->byteCount());

                backend->getTensorAsync(*destination, downloaded.data(), 0, downloaded.size());
                backend->synchronize();

                // const bool matches = std::memcmp(
                //                          downloaded.data(),
                //                          source->tensor()->data,
                //                          downloaded.size()) == 0;

                // JOB_LOG_WARN("[JobModel WEIGHTS] {} bytes={} roundTrip={}",
                //              source->name(), downloaded.size(), matches ? "PASS" : "FAIL");
            }
        }

        // ModelWeights is a borrowing index into its context: rebind only
        // after all tensors are fully materialized on the selected backend.
        ModelWeights deviceWeights;

        if (!deviceWeights.loadFromContext(*deviceCtx, m_config)) {
#ifndef NDEBUG
            JOB_LOG_ERROR("[JobModel] Failed to bind device-resident model weights");
#endif
            return false;
        }

        m_weights = std::move(deviceWeights);
        m_weightBuffer = std::move(deviceBuffer);
        weightCtx = std::move(deviceCtx);
#ifndef NDEBUG
        JOB_LOG_INFO("[JobModel] Materialized {} tensors in {} backend bytes", tensorCount, totalBytes);
#endif
    }

    m_weightCtx = std::move(weightCtx);
    return true;
}

bool JobModel::createKvCache(uint32_t maxContextLength)
{
    const auto *bufferType = m_device.bufferType();

    if (!bufferType || !bufferType->isValid()) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobModel] Resolved device does not expose a valid buffer type");
#endif
        return false;
    }

    const uint32_t contextLength = maxContextLength > 0 ?
                                       maxContextLength :
                                       m_config.transformerConfig().contextLength();

    if (contextLength == 0) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobModel] Cannot create KV cache with zero context length");
#endif
        return false;
    }

    try {
        m_kvCache = KvCache::createUniq(m_config, *bufferType, contextLength, ggml::JobGgmlType::F16);
    } catch (const std::exception &error) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobModel] Failed to create KV cache: {}", error.what());
#endif
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
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobModel] Failed to create compute context init parameters");
#endif
        return false;
    }

    m_computeCtx = ggml::JobGgmlContext::createUniq(*initParams);

    if (!m_computeCtx || !m_computeCtx->isValid()) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobModel] Failed to create compute metadata context");
#endif
        m_computeCtx.reset();
        return false;
    }

    return true;
}

bool JobModel::createGraphBuilder()
{
    if (!m_kvCache) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobModel] Cannot create graph builder without a KV cache");
#endif
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
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobModel] Failed to create architecture graph builder: {}", error.what());
#endif
        m_graphBuilder.reset();
        return false;
    }

    return m_graphBuilder != nullptr;
}

ggml::JobGgmlTensor::UPtr JobModel::createInputTensor(std::span<const int32_t> tokens,
                                                      ggml::JobGgmlBackendBuffer::Ptr &buffer)
{
#ifndef NDEBUG
    if (!m_computeCtx || !m_computeCtx->isValid())
        throw std::runtime_error{"JobModel requires a valid compute context"};

    if (tokens.empty())
        throw std::invalid_argument{"JobModel input token span cannot be empty"};
#endif
    auto *bufferType = m_device.bufferType();
#ifndef NDEBUG
    if (!bufferType || !bufferType->isValid())
        throw std::runtime_error{"JobModel device does not expose a valid buffer type"};
#endif
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

// int32_t JobModel::sampleLastToken(ggml::JobGgmlCGraph &graph,
//                                   Sampler &sampler,
//                                   std::span<const int32_t> contextTokens)
// {
//     auto logits = graph.tensor("logits");

//     if (!logits || !logits->isValid()) {
//         JOB_LOG_ERROR("[JobModel] Graph does not contain valid logits");
//         return -1;
//     }

//     const int64_t vocabSize = logits->extent(0);
//     const int64_t sequenceLength = logits->extent(1);

//     if (vocabSize <= 0 || sequenceLength <= 0) {
//         JOB_LOG_ERROR("[JobModel] Logits tensor has invalid dimensions");
//         return -1;
//     }

//     if (logits->type() != ggml::JobGgmlType::F32) {
//         JOB_LOG_ERROR("[JobModel] Sampler requires F32 logits");
//         return -1;
//     }

//     const std::size_t rowBytes = static_cast<std::size_t>(vocabSize) * sizeof(float);
//     const std::size_t offset = static_cast<std::size_t>(sequenceLength - 1) * logits->stride(1);

//     if (logits->stride(0) != sizeof(float) ||
//         offset > logits->byteCount() ||
//         rowBytes > logits->byteCount() - offset) {
//         JOB_LOG_ERROR("[JobModel] Logits tensor has an unsupported layout");
//         return -1;
//     }

//     ggml::JobGgmlTensorData logitsData{logits->tensor()};
//     // CPU
//     if (logitsData.isHostAccessible()) {
//         auto lastLogits = ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor *>(logits->tensor()), m_computeCtx.get())->view1d(vocabSize, offset);
//         if (!lastLogits || !lastLogits->isValid()) {
//             JOB_LOG_ERROR("[JobModel] Failed to create final-token logits view");
//             return -1;
//         }

//         return sampler.sample(*lastLogits, contextTokens);
//     }

//     // Download the first and last logits rows to verify whether CUDA
//     // produced values for the sequence, or whether the output is empty.
//     auto backend = m_scheduler.tensorBackend(*logits);
//     if (!backend || !backend->isValid()) {
//         JOB_LOG_ERROR("[JobModel] Failed to resolve logits backend");
//         return -1;
//     }

//     std::vector<float> firstLogits(static_cast<std::size_t>(vocabSize));
//     std::vector<float> hostLogits(static_cast<std::size_t>(vocabSize));

//     try {
//         backend->getTensorAsync(*logits, firstLogits.data(), 0, rowBytes);
//         backend->getTensorAsync(*logits, hostLogits.data(), offset, rowBytes);
//         backend->synchronize();
//     } catch (const std::exception &error) {
//         JOB_LOG_ERROR("[JobModel] Failed to download logits: {}", error.what());
//         return -1;
//     }
//     return sampler.sample(std::span<const float>{hostLogits}, contextTokens);
// }
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
    if (!isLoaded()|| promptTokens.empty()) {
        JOB_LOG_ERROR("[JobModel] Cannot generate: model is not loaded OR from an empty prompt") ;
        return {};
    }

    if (maxNewTokens <= 0)
        return std::vector<int32_t>{promptTokens.begin(), promptTokens.end()};

    m_kvCache->resetPosition();
    std::vector<int32_t> outputTokens{ promptTokens.begin(), promptTokens.end() };
    outputTokens.reserve(promptTokens.size() + static_cast<std::size_t>(maxNewTokens));

    Sampler sampler{samplerConfig};
    uint32_t nPast = 0;

    // ------------------------------------------------------------
    // Prefill
    // ------------------------------------------------------------
    m_scheduler.synchronize();  // NEEDED ?
    m_scheduler.reset();        // NEEDED ?
    m_computeCtx->reset();

    ggml::JobGgmlBackendBuffer::Ptr inputBuffer;
    ggml::JobGgmlTensor::UPtr inputTensor;

    try {
        inputTensor = createInputTensor(promptTokens, inputBuffer);
    } catch (const std::exception &error) {
        JOB_LOG_ERROR("[JobModel] Failed to create prefill input: {}", error.what());
        return {};
    }

    auto graph = m_graphBuilder->buildForward(*m_computeCtx,
                                              *inputTensor,
                                              nPast,
                                              ggml::JobGgmlType::F32);

    if (!graph || !graph->isValid()) {
        JOB_LOG_ERROR("[JobModel] Failed to build prefill graph");
        return {};
    }

    if (m_scheduler.computeGraph(*graph) != ggml::JobGgmlStatus::Success) {
        JOB_LOG_ERROR("[JobModel] Prefill graph execution failed");
        return {};
    }

    m_scheduler.synchronize();
    int32_t nextToken = sampleLastToken(*graph, sampler, outputTokens);
    if (nextToken < 0)
        return {};

    outputTokens.push_back(nextToken);
    nPast = static_cast<uint32_t>(promptTokens.size());
    m_kvCache->advance(static_cast<uint32_t>(promptTokens.size()));

    // ------------------------------------------------------------
    // Decode
    // ------------------------------------------------------------
    for (int32_t step = 1; step < maxNewTokens; ++step) {
        // Release scheduler references before resetting compute metadata.
        // m_scheduler.synchronize();
        m_scheduler.reset();
        m_computeCtx->reset();

        const int32_t token = nextToken;
        ggml::JobGgmlBackendBuffer::Ptr stepInputBuffer;
        ggml::JobGgmlTensor::UPtr stepInput;

        try {
            stepInput = createInputTensor(std::span<const int32_t>{&token, 1}, stepInputBuffer);
        } catch (const std::exception &error) {
            JOB_LOG_ERROR("[JobModel] Failed to create decode input at step {}: {}", step, error.what());
            break;
        }

        auto stepGraph = m_graphBuilder->buildForward(*m_computeCtx,
                                                      *stepInput,
                                                      nPast,
                                                      ggml::JobGgmlType::F32);

        if (!stepGraph || !stepGraph->isValid()) {
            JOB_LOG_ERROR("[JobModel] Failed to build decode graph at step {}", step);
            break;
        }

        if (m_scheduler.computeGraph(*stepGraph) != ggml::JobGgmlStatus::Success) {
            JOB_LOG_ERROR("[JobModel] Decode graph execution failed at step {}", step);
            break;
        }
        m_scheduler.synchronize();

        nextToken = sampleLastToken(*stepGraph, sampler, outputTokens);
        if (nextToken < 0)
            break;

        // Okay we need to look at the tokens and see of we are at the end thus stoping the
        if(isStopToken(nextToken))
            break;



        outputTokens.push_back(nextToken);

        ++nPast;
        m_kvCache->advance();
    }

    return outputTokens;
}

bool job::model::JobModel::isStopToken(int32_t tokenId) const noexcept noexcept
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