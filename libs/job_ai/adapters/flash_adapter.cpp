#include "flash_adapter.h"
#include "flash_attention.h"
namespace job::ai::adapters {

FlashAdapter::FlashAdapter()
{
}

AdapterType FlashAdapter::type() const
{
    return AdapterType::Flash;
}

std::string FlashAdapter::name() const
{
    return "FlashAttention";
}

void FlashAdapter::adapt(threads::ThreadPool &pool,
                         const cords::AttentionShape &shape, const cords::ViewR &sources, const cords::ViewR &targets, const cords::ViewR &values, cords::ViewR &output,
                         [[maybe_unused]] const AdapterCtx &ctx)
{
    job::ai::comp::flashAttentionForward(
        pool,
        shape.seq,
        shape.dim,
        targets.data(), // Q
        sources.data(), // K
        values.data(),  // V
        output.data()   // O
        );
}

}
