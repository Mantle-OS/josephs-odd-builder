#pragma once

#include "iadapter.h"
#include "lowrank_config.h"


namespace job::ai::adapters {

class LowRankAdapter : public IAdapter {
public:
    static std::unique_ptr<LowRankAdapter> unique(LowRankConfig cfg = {})
    {
        return std::make_unique<LowRankAdapter>(cfg);
    }
    explicit LowRankAdapter(LowRankConfig cfg);

    [[nodiscard]] AdapterType type() const override;
    [[nodiscard]] std::string name() const override;

    void adapt(job::threads::ThreadPool &pool,
               const cords::AttentionShape &shape, const cords::ViewR &sources, const cords::ViewR &targets, const cords::ViewR &values, cords::ViewR &output,
               [[maybe_unused]] const AdapterCtx &ctx
               ) override;

private:
    LowRankConfig m_cfg;
};

} // namespace
