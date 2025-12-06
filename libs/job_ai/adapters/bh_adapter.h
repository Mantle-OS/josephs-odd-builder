#pragma once

#include <memory>
#include <string>

#include <job_barnes_hut_calculator.h>
#include <job_scheduler_types.h>

#include <vec3f.h>

#include "iadapter.h"
#include "adapter_types.h"

#include "bh_config.h"

namespace job::ai::adapters {

class BhAdapter : public IAdapter {
public:
    static std::unique_ptr<BhAdapter> unique(BhConfig cfg = {})
    {
        return std::make_unique<BhAdapter>(cfg);
    }

    explicit BhAdapter(BhConfig cfg = {});

    [[nodiscard]] AdapterType type() const override;
    [[nodiscard]] std::string name() const override;

    void adapt(job::threads::ThreadPool &pool,
        const cords::AttentionShape &shape,
        const cords::ViewR &sources, const cords::ViewR &targets, const cords::ViewR &values, cords::ViewR &output,
        const AdapterCtx &ctx) override;

private:
    BhConfig m_cfg;
};

} // namespace job::ai::adapters
