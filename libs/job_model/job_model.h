#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>


#include <job_ggml_backend_sched.h>
#include <job_ggml_backend_buffer.h>
#include <job_ggml_context.h>
#include <job_ggml_device.h>
#include <job_ggml_enums.h>
#include <job_gguf.h>

#include <job_token.h>

/////
#include <array>
#include <cstring>

#include <ggml-backend.h>
#include <ggml.h>
#include <job_logger.h>

#include <chrono>
#include <cstdarg>
///



#include "config/model_config.h"
#include "config/sampler_config.h"

#include "graph/graph_builder.h"

#include "weights/model_weights.h"

#include "kv/kv_cache.h"

#include "sample/sampler.h"

#include "jobmodel_export.h"
namespace job::model {


enum class StageType: std::uint8_t {
    Reset,
    Input,
    Build,
    ComputeAlloc,
    Compute,
    Sync,
    Sample,
    Idle
};

enum class InferencePhase : std::uint8_t {
    Prefill,
    Decode,
};

struct StageTiming
{
    using Clock = std::chrono::steady_clock;

    double              totalMs{0.0};
    std::uint64_t       calls{0};
    bool                running{false};
    Clock::time_point   startedAt{};

    void start() noexcept
    {
        if (running)
            return;

        startedAt = Clock::now();
        running = true;
    }

    void stop() noexcept
    {
        if (!running)
            return;

        const auto endedAt = Clock::now();
        totalMs += std::chrono::duration<double, std::milli>(endedAt - startedAt).count();
        ++calls;
        running = false;
    }

    [[nodiscard]] double averageMs() const noexcept
    {
        return calls ? totalMs / static_cast<double>(calls) : 0.0;
    }

    void reset() noexcept
    {
        totalMs = 0.0;
        calls = 0;
        running = false;
        startedAt = {};
    }
};


struct InferenceTiming
{
    struct Phase
    {
        StageTiming reset;
        StageTiming input;
        StageTiming build;
        StageTiming computeAlloc;
        StageTiming compute;
        StageTiming sync;
        StageTiming sample;

    };

    Phase prefill;
    Phase decode;

    void reset() noexcept
    {
        prefill = {};
        decode = {};
    }
};

class JOBMODEL_EXPORT JobModel
{
public:
    using Ptr  = std::shared_ptr<JobModel>;
    using WPtr = std::weak_ptr<JobModel>;
    using UPtr = std::unique_ptr<JobModel>;

    JobModel(ggml::JobGgmlDevice &device, ggml::JobGgmlBackendSched &scheduler);
    ~JobModel() = default;

    [[nodiscard]] static Ptr createShared(ggml::JobGgmlDevice &device,
                                          ggml::JobGgmlBackendSched &scheduler)
    {
        return std::make_shared<JobModel>(device, scheduler);
    }

    [[nodiscard]] static UPtr createUniq(ggml::JobGgmlDevice &device,
                                         ggml::JobGgmlBackendSched &scheduler)
    {
        return std::make_unique<JobModel>(device, scheduler);
    }

    JobModel(const JobModel &) = delete;
    JobModel &operator=(const JobModel &) = delete;
    JobModel(JobModel &&) = delete;
    JobModel &operator=(JobModel &&) = delete;

    // Loads configuration from the GGUF reader path.
    [[nodiscard]] bool load(const std::filesystem::path &ggufPath, uint32_t maxContextLength = 0);

    // Loads using an explicitly supplied model configuration/preset.
    [[nodiscard]] bool load(const std::filesystem::path &ggufPath, ModelConfig config, uint32_t maxContextLength = 0);

    [[nodiscard]] std::vector<int32_t> generate(std::span<const int32_t> promptTokens, int32_t maxNewTokens, const SamplerConfig &samplerConfig = {});

    [[nodiscard]] bool isLoaded() const noexcept;

    [[nodiscard]] ModelConfig &config() noexcept { return m_config; }
    [[nodiscard]] const ModelConfig &config() const noexcept { return m_config; }

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


    [[nodiscard]] token::JobToken *tokenizer()const { return m_tokenizer;}
    void setTokenizer(token::JobToken *tokenizer) { m_tokenizer = tokenizer;}

    // [[nodiscard]] StageTiming stageTimmer(){return m_stageTimmer;}
    [[nodiscard]] const InferenceTiming &inferenceTiming() const noexcept
    {
        return m_inferenceTime;
    }

    void startTime(InferencePhase phase, StageType type) noexcept
    {
        updateTime(phase, type);
    }

    void stopTime(InferencePhase phase, StageType type) noexcept
    {
        updateTime(phase, type, true);
    }

    // [[nodiscard]] ggml::JobGguf *gguf(){
    //     return m_gguf;
    // }
    // void setGguf(ggml::JobGguf *gguf) { m_gguf = gguf;}


private:
    [[nodiscard]] bool loadConfigFromGguf(const std::filesystem::path &ggufPath);
    [[nodiscard]] bool loadWeights(const std::filesystem::path &ggufPath);
    [[nodiscard]] bool createKvCache(uint32_t maxContextLength);
    [[nodiscard]] bool createComputeContext();
    [[nodiscard]] bool createGraphBuilder();

    [[nodiscard]] ggml::JobGgmlTensor::UPtr createInputTensor(std::span<const int32_t> tokens,
                                                              ggml::JobGgmlBackendBuffer::Ptr &buffer);

    [[nodiscard]] int32_t sampleLastToken(ggml::JobGgmlCGraph &graph, Sampler &sampler, std::span<const int32_t> contextTokens);

    [[nodiscard]] bool isStopToken(int32_t tokenId) const noexcept;

    // [[nodiscard]] bool isToolToken(int32_t tokenId) const noexcept;
    void updateTime(InferencePhase phase, StageType type, bool stop = false) noexcept
    {
        auto &timing = phase == InferencePhase::Prefill
                           ? m_inferenceTime.prefill
                           : m_inferenceTime.decode;

        StageTiming *stage = nullptr;

        switch (type) {
        case StageType::Reset:          stage = &timing.reset;          break;
        case StageType::Input:          stage = &timing.input;          break;
        case StageType::Build:          stage = &timing.build;          break;
        case StageType::ComputeAlloc:   stage = &timing.computeAlloc;   break;
        case StageType::Compute:        stage = &timing.compute;        break;
        case StageType::Sync:           stage = &timing.sync;           break;
        case StageType::Sample:         stage = &timing.sample;         break;
        case StageType::Idle:    return;
        }

        if (stop)
            stage->stop();
        else
            stage->start();
    }


private:

    static void debugKvPosition(const ggml_tensor *tensor,
                                const char *label,
                                std::size_t position)
    {
        if (!tensor || !tensor->buffer || tensor->type != GGML_TYPE_F16)
            return;

        if (position >= static_cast<std::size_t>(tensor->ne[1]))
            return;

        constexpr std::size_t kElements = 8;

        std::array<ggml_fp16_t, kElements> raw{};

        const std::size_t offset = position * tensor->nb[1];

        ggml_backend_tensor_get(
            tensor,
            raw.data(),
            offset,
            sizeof(raw));

        std::array<float, kElements> values{};

        for (std::size_t i = 0; i < kElements; ++i)
            values[i] = ggml_fp16_to_fp32(raw[i]);

        JOB_LOG_WARN(
            "[KV DEBUG] {} pos={} values=[{}, {}, {}, {}, {}, {}, {}, {}]",
            label, position,
            values[0], values[1], values[2], values[3],
            values[4], values[5], values[6], values[7]);
    }

    // Meta data timming
    // StageTiming m_stageTimmer;       // OWNED
    InferenceTiming m_inferenceTime; // OWNED


    ggml::JobGgmlDevice         &m_device;          // Borrowed.
    ggml::JobGgmlBackendSched   &m_scheduler;       // Borrowed.
    token::JobToken             *m_tokenizer    = nullptr; // Borrowed
    // ggml::JobGguf               *m_gguf         = nullptr; // Borrowed

    ModelConfig                 m_config;           // Owned
    ModelWeights                m_weights;          // Owned
    ggml::JobGgmlContext::UPtr  m_weightCtx;        // OWNED
    ggml::JobGgmlBackendBuffer::Ptr m_weightBuffer;  // OWNED, accelerator weights
    ggml::JobGgmlContext::UPtr  m_computeCtx;       // OWNED
    KvCache::UPtr               m_kvCache;          // OWNED
    GraphBuilder::UPtr          m_graphBuilder;     // OWNED
};

} // namespace job::model
