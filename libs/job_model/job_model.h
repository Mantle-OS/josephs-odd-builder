#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

#include <job_ggml_backend_sched.h>
#include <job_ggml_context.h>
#include <job_ggml_device.h>
#include <job_ggml_enums.h>

#include "config/device_config.h"
#include "config/model_config.h"
#include "config/sampler_config.h"
#include "graph/graph_builder.h"
#include "jobmodel_export.h"
#include "kv/kv_cache.h"
#include "sample/sampler.h"
#include "weights/model_weights.h"

namespace job::model {

class JOBMODEL_EXPORT JobModel
{
public:
    using Ptr  = std::shared_ptr<JobModel>;
    using WPtr = std::weak_ptr<JobModel>;
    using UPtr = std::unique_ptr<JobModel>;

    JobModel(ggml::JobGgmlDevice &device,
             ggml::JobGgmlBackendSched &scheduler,
             DeviceConfig deviceConfig = {});

    ~JobModel() = default;

    [[nodiscard]] static Ptr createShared(ggml::JobGgmlDevice &device,
                                          ggml::JobGgmlBackendSched &scheduler,
                                          DeviceConfig deviceConfig = {})
    {
        return std::make_shared<JobModel>(device,
                                          scheduler,
                                          std::move(deviceConfig));
    }

    [[nodiscard]] static UPtr createUniq(ggml::JobGgmlDevice &device,
                                         ggml::JobGgmlBackendSched &scheduler,
                                         DeviceConfig deviceConfig = {})
    {
        return std::make_unique<JobModel>(device,
                                          scheduler,
                                          std::move(deviceConfig));
    }

    JobModel(const JobModel &) = delete;
    JobModel &operator=(const JobModel &) = delete;
    JobModel(JobModel &&) = delete;
    JobModel &operator=(JobModel &&) = delete;

    // Loads configuration from the GGUF reader path.
    [[nodiscard]] bool load(const std::filesystem::path &ggufPath,
                            uint32_t maxContextLength = 0);

    // Loads using an explicitly supplied model configuration/preset.
    [[nodiscard]] bool load(const std::filesystem::path &ggufPath,
                            ModelConfig config,
                            uint32_t maxContextLength = 0);

    [[nodiscard]] std::vector<int32_t> generate(std::span<const int32_t> promptTokens, int32_t maxNewTokens, const SamplerConfig &samplerConfig = {});

    [[nodiscard]] bool isLoaded() const noexcept;

    [[nodiscard]] ModelConfig &config() noexcept { return m_config; }
    [[nodiscard]] const ModelConfig &config() const noexcept { return m_config; }

    [[nodiscard]] DeviceConfig &deviceConfig() noexcept { return m_deviceConfig; }
    [[nodiscard]] const DeviceConfig &deviceConfig() const noexcept { return m_deviceConfig; }

    [[nodiscard]] ModelWeights &weights() noexcept { return m_weights; }
    [[nodiscard]] const ModelWeights &weights() const noexcept { return m_weights; }

    [[nodiscard]] KvCache *kvCache() noexcept { return m_kvCache.get(); }
    [[nodiscard]] const KvCache *kvCache() const noexcept { return m_kvCache.get(); }

    [[nodiscard]] GraphBuilder *graphBuilder() noexcept { return m_graphBuilder.get(); }
    [[nodiscard]] const GraphBuilder *graphBuilder() const noexcept { return m_graphBuilder.get(); }

    [[nodiscard]] ggml::JobGgmlDevice &device() noexcept { return m_device; }
    [[nodiscard]] const ggml::JobGgmlDevice &device() const noexcept { return m_device; }

    [[nodiscard]] ggml::JobGgmlBackendSched &scheduler() noexcept { return m_scheduler; }
    [[nodiscard]] const ggml::JobGgmlBackendSched &scheduler() const noexcept { return m_scheduler; }

    [[nodiscard]] ggml::JobGgmlContext *weightContext() noexcept { return m_weightCtx.get(); }
    [[nodiscard]] const ggml::JobGgmlContext *weightContext() const noexcept { return m_weightCtx.get(); }

    [[nodiscard]] ggml::JobGgmlContext *computeContext() noexcept { return m_computeCtx.get(); }
    [[nodiscard]] const ggml::JobGgmlContext *computeContext() const noexcept { return m_computeCtx.get(); }

    void reset() noexcept;

private:
    [[nodiscard]] bool loadConfigFromGguf(const std::filesystem::path &ggufPath);
    [[nodiscard]] bool loadWeights(const std::filesystem::path &ggufPath);
    [[nodiscard]] bool createKvCache(uint32_t maxContextLength);
    [[nodiscard]] bool createComputeContext();
    [[nodiscard]] bool createGraphBuilder();

    [[nodiscard]] ggml::JobGgmlTensor::UPtr createInputTensor(std::span<const int32_t> tokens,
                                                              ggml::JobGgmlBackendBuffer::Ptr &buffer);

    [[nodiscard]] int32_t sampleLastToken(ggml::JobGgmlCGraph &graph,
                                          Sampler &sampler,
                                          std::span<const int32_t> contextTokens);

private:
    ggml::JobGgmlDevice       &m_device;    // Borrowed.
    ggml::JobGgmlBackendSched &m_scheduler; // Borrowed.

    DeviceConfig m_deviceConfig;
    ModelConfig  m_config;
    ModelWeights m_weights;

    ggml::JobGgmlContext::UPtr m_weightCtx;
    ggml::JobGgmlContext::UPtr m_computeCtx;

    KvCache::UPtr      m_kvCache;
    GraphBuilder::UPtr m_graphBuilder;
};

} // namespace job::model