#pragma once

#include "iadapter.h"
#include "dense_config.h"


namespace job::ai::adapters {

class DenseAdapter : public IAdapter {
public:

    [[nodiscard]] static std::unique_ptr<DenseAdapter> unique(DenseConfig cfg = {})
    {
        return std::make_unique<DenseAdapter>(cfg);
    }

    explicit DenseAdapter(DenseConfig cfg);
    [[nodiscard]] AdapterType type() const override;
    [[nodiscard]] std::string name() const override;

    void adaptParallel(
        job::threads::ThreadPool &pool,
        const cords::AttentionShape &shape,
        const cords::ViewR &sources, // K [B, S, D]
        const cords::ViewR &targets, // Q [B, S, D]
        const cords::ViewR &values,  // V [B, S, D]
        cords::ViewR &output,        // O [B, S, D]
        const AdapterCtx &ctx
        ) override;

    void adapt(
        job::threads::ThreadPool &pool,
        const cords::AttentionShape &shape,
        const cords::ViewR &sources, // K [B, S, D]
        const cords::ViewR &targets, // Q [B, S, D]
        const cords::ViewR &values,  // V [B, S, D]
        cords::ViewR &output,        // O [B, S, D]
        const AdapterCtx &ctx
    ) override;


    void apply(const cords::AttentionShape &shape, const cords::ViewR &sources, const cords::ViewR &targets, const cords::ViewR &values, cords::ViewR &output, size_t size);



private:
    DenseConfig m_cfg;
};

} // namespace
