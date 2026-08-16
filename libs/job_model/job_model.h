#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <job_ggml_backend.h>
#include <job_ggml_backend_sched.h>
#include <job_ggml_context.h>
#include <job_ggml_tensor.h>
#include <job_gguf.h>

#include "config/model_config.h"
#include "config/sampler_config.h"
#include "graph/compute_graph_builder.h"
#include "job_ggml.h"
#include "jobmodel_export.h"
#include "kv/kv_cache.h"
#include "sampler.h"
#include "weights/model_weights.h"

namespace job::model {

class JOBMODEL_EXPORT JobModel {
public:
    using Ptr  = std::shared_ptr<JobModel>;
    using UPtr = std::unique_ptr<JobModel>;

    JobModel();
    ~JobModel();

    JobModel(const JobModel&) = delete;
    JobModel& operator=(const JobModel&) = delete;
    JobModel(JobModel&&) noexcept;
    JobModel& operator=(JobModel&&) noexcept;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<JobModel>(); }
    [[nodiscard]] static UPtr createUniq() { return std::make_unique<JobModel>(); }

    // Load model weights, config, and initialize compute/backend context from GGUF
    [[nodiscard]] bool load(const std::filesystem::path& ggufPath, uint32_t maxContextLength = 0);

    // take in a vendor file example the qwen "preset"
    [[nodiscard]] bool load(const std::filesystem::path& ggufPath, ModelConfig config, uint32_t maxContextLength = 0);


    // Generate tokens given a prompt sequence
    [[nodiscard]] std::vector<int32_t> generate(
        std::span<const int32_t> promptTokens,
        int32_t maxNewTokens,
        const SamplerConfig& samplerConfig = {});

    // Component inspection
    [[nodiscard]] bool isLoaded() const noexcept;
    [[nodiscard]] const ModelConfig& config() const noexcept { return m_config; }
    [[nodiscard]] const ModelWeights& weights() const noexcept { return m_weights; }
    [[nodiscard]] KvCache& kvCache() noexcept { return m_kvCache; }  

    void reset();

private:
    // Top-level GGML subsystem facade stored as a movable unique_ptr
    ggml::JobGgml::UPtr m_ggmlSubsystem;

    ModelConfig             m_config;
    ModelWeights            m_weights;
    KvCache                 m_kvCache;

    ggml::JobGgmlContext::UPtr m_weightCtx;
    ggml::JobGgmlContext::UPtr m_computeCtx;

    ggml::JobGgmlBackend::Ptr      m_backend;
    ggml::JobGgmlBackendSched::Ptr m_scheduler;
};

} // namespace job::model