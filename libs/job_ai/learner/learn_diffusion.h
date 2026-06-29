#pragma once

#include <job_thread_pool.h>

#include "ilearn.h"
#include "learn_config.h"
namespace job::ai::learn {

class DiffusionLearn final : public ILearn {
public:
    explicit DiffusionLearn(const LearnConfig &cfg = LearnPresets::DiffusionConfig(),
                            threads::ThreadPool::Ptr pool = nullptr);

    [[nodiscard]] float learn(const evo::Genome &genome) override;
    [[nodiscard]] uint32_t inputDimension() const noexcept override;
    [[nodiscard]] uint32_t outputDimension() const noexcept override;

    [[nodiscard]] static std::unique_ptr<ILearn> create(const LearnConfig &cfg,
                                                        threads::ThreadPool::Ptr pool);

private:
    // ggml context, graph, backend scheduler
    // UNet layers, noise scheduler
    // AdamW optimizer state
};
}