#pragma once

#include "adapter_types.h"
#include "iadapter.h"

namespace job::ai::adapters {

class FlashAdapter : public IAdapter {
public:
    explicit FlashAdapter();

    [[nodiscard]] AdapterType type() const override;
    [[nodiscard]] std::string name() const override;

    void adapt(threads::ThreadPool &pool,
               const cords::AttentionShape &shape, const cords::ViewR &sources, const cords::ViewR &targets, const cords::ViewR &values,
               cords::ViewR &output,
               [[maybe_unused]] const AdapterCtx &ctx
               ) override;

    static std::unique_ptr<FlashAdapter> unique()
    {
        return std::make_unique<FlashAdapter>();
    }
};
}
