#pragma once

#include <memory>
#include <string>

#include <job_barnes_hut_calculator.h>

#include <vec3f.h>

#include "iadapter.h"
#include "adapter_types.h"

#include "bh_config.h"

namespace job::ai::adapters {

class BhAdapter : public IAdapter {
public:
    using Solver = science::BarnesHutForceCalculator<BhTraits::Body, BhTraits::Vec3, BhTraits::Real>;

    static std::unique_ptr<BhAdapter> unique(BhConfig cfg = {})
    {
        return std::make_unique<BhAdapter>(cfg);
    }

    explicit BhAdapter(BhConfig cfg = {});

    [[nodiscard]] AdapterType type() const override;
    [[nodiscard]] std::string name() const override;

    void adaptParallel(threads::ThreadPool &pool,
                               const cords::AttentionShape &shape,
                               const cords::ViewR &sources,
                               const cords::ViewR &targets,
                               const cords::ViewR &values,
                               cords::ViewR &output,
                               [[maybe_unused]] const AdapterCtx &ctx) override;


    void adapt(threads::ThreadPool &pool,
        const cords::AttentionShape &shape,
        const cords::ViewR &sources, const cords::ViewR &targets, const cords::ViewR &values, cords::ViewR &output,
        const AdapterCtx &ctx) override;


    void apply(threads::ThreadPool &pool,
               const cords::AttentionShape &shape,
               const cords::ViewR &sources,
               cords::ViewR &output,
               size_t i);



private:
    BhConfig m_cfg;
};

} // namespace job::ai::adapters


// AI Analogy: It is purely Contextual Clustering, not "Retrieval."Is this "Bad"?For a Language Model, it might be limiting because it can't say "I am looking for a Verb" ($Q$). It just says "Verbs clump together" ($K$).But combined with your standard DenseLayer, it might act as a powerful Global Mixing operator that standard attention misses.
