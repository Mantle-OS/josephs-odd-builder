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

void FlashAdapter::adaptParallel(threads::ThreadPool &pool,
                         const cords::AttentionShape &shape, const cords::ViewR &sources, const cords::ViewR &targets, const cords::ViewR &values, cords::ViewR &output,
                         [[maybe_unused]] const AdapterCtx &ctx)
{
    job::ai::comp::flashParallelAttentionForward(
        pool,
        shape.seq,      // N
        shape.dim,      // D
        targets.data(), // Q
        sources.data(), // K
        values.data(),  // V
        output.data(),   // O
        1.0f / std::sqrt(float(shape.dim))
        );
}

void FlashAdapter::adapt([[maybe_unused]] threads::ThreadPool &pool,
                         const cords::AttentionShape &shape, const cords::ViewR &sources, const cords::ViewR &targets, const cords::ViewR &values, cords::ViewR &output,
                         [[maybe_unused]] const AdapterCtx &ctx)
{
    job::ai::comp::flashAttentionForward(
        shape.seq,      // N
        shape.dim,      // D
        targets.data(), // Q
        sources.data(), // K
        values.data(),  // V
        output.data(),   // O
        1.0f / std::sqrt(float(shape.dim))
        );
}

}
