#include "stencil_adapter.h"
#include <cmath>
#include <vector>
#include "transpose.h"
#include "aligned_allocator.h"

namespace job::ai::adapters {

StencilAdapter::StencilAdapter(StencilConfig cfg) :
    m_cfg(cfg)
{
}

AdapterType StencilAdapter::type() const
{
    return AdapterType::Stencil;
}

std::string StencilAdapter::name() const
{
    return "Stencil (Reaction-Diffusion)";
}

void StencilAdapter::adaptParallel(threads::ThreadPool &pool,
                           const cords::AttentionShape &shape, const cords::ViewR &sources,
                           [[maybe_unused]]const cords::ViewR &targets,
                           [[maybe_unused]]const cords::ViewR &values,
                           cords::ViewR &output,
                           [[maybe_unused]]const AdapterCtx &ctx)
{
    const int B = static_cast<int>(shape.batch);
    const int S = static_cast<int>(shape.seq);
    const int D = static_cast<int>(shape.dim);

    int width = static_cast<int>(std::sqrt(S));
    if (width * width < S)
        width++;
    int height = (S + width - 1) / width;

    size_t elementsPerBatch = static_cast<size_t>(S) * D;

    // FIXME: In v3, ask Workspace for this buffer to avoid malloc.
     cords::AiWeights scratch(B * elementsPerBatch);

    threads::parallel_for(pool, size_t{0}, size_t(B), [&](size_t b) {
        const float *srcBatch = sources.data() + (b * elementsPerBatch);
        float *scratchBatch   = scratch.data() + (b * elementsPerBatch);
        comp::transpose(srcBatch, scratchBatch, S, D);
    });

    size_t totalSims = static_cast<size_t>(B) * D;

    threads::parallel_for(pool, size_t{0}, totalSims, [&](size_t idx) {
        int b = static_cast<int>(idx / D);
        int d = static_cast<int>(idx % D);

        float *channelData = scratch.data() + (b * elementsPerBatch) + (d * S);

        science::JobStencilGrid2D<float> grid(pool, width, height, false);

        std::memcpy(grid.data(), channelData, S * sizeof(float));

        float rate = m_cfg.diffusionRate;
        auto realKernel = [rate, this](int x, int y, science::GridReader2D<float> view) -> float {
            float center = view(x, y);
            float sum = 0.0f;
            sum += view.at(x + 1, y, m_cfg.boundary);
            sum += view.at(x - 1, y, m_cfg.boundary);
            sum += view.at(x, y + 1, m_cfg.boundary);
            sum += view.at(x, y - 1, m_cfg.boundary);
            return center + rate * (sum - 4.0f * center);
        };

        grid.step_n(m_cfg.steps, realKernel);
        std::memcpy(channelData, grid.data(), S * sizeof(float));
    });

    threads::parallel_for(pool, size_t{0}, size_t(B), [&](size_t b) {
        const float *scratchBatch = scratch.data() + (b * elementsPerBatch);
        float *outBatch = output.data()  + (b * elementsPerBatch);
        comp::transpose(scratchBatch, outBatch, D, S);
    });
}

void StencilAdapter::adapt(threads::ThreadPool &pool,
                           const cords::AttentionShape &shape,
                           const cords::ViewR &sources,
                           [[maybe_unused]]const cords::ViewR &targets,
                           [[maybe_unused]]const cords::ViewR &values,
                           cords::ViewR &output,
                           [[maybe_unused]]const AdapterCtx &ctx)
{
    const int B = static_cast<int>(shape.batch);
    const int S = static_cast<int>(shape.seq);
    const int D = static_cast<int>(shape.dim);

    int width = static_cast<int>(std::sqrt(S));
    if (width * width < S)
        width++;
    int height = (S + width - 1) / width;

    size_t elementsPerBatch = static_cast<size_t>(S) * D;

    // FIXME: In v3, ask Workspace for this buffer to avoid malloc.
    cords::AiWeights scratch(B * elementsPerBatch);
    for(size_t b = 0; b < size_t(B); ++b){
        const float *srcBatch = sources.data() + (b * elementsPerBatch);
        float *scratchBatch   = scratch.data() + (b * elementsPerBatch);
        comp::transpose(srcBatch, scratchBatch, S, D);
    }

    size_t totalSims = static_cast<size_t>(B) * D;

    for(size_t idx = 0; idx <= totalSims; ++idx){
        int b = static_cast<int>(idx / D);
        int d = static_cast<int>(idx % D);

        float *channelData = scratch.data() + (b * elementsPerBatch) + (d * S);

        science::JobStencilGrid2D<float> grid(pool, width, height, false);

        std::memcpy(grid.data(), channelData, S * sizeof(float));

        float rate = m_cfg.diffusionRate;
        auto realKernel = [rate, this](int x, int y, science::GridReader2D<float> view) -> float {
            float center = view(x, y);
            float sum = 0.0f;
            sum += view.at(x + 1, y, m_cfg.boundary);
            sum += view.at(x - 1, y, m_cfg.boundary);
            sum += view.at(x, y + 1, m_cfg.boundary);
            sum += view.at(x, y - 1, m_cfg.boundary);
            return center + rate * (sum - 4.0f * center);
        };

        grid.step_n(m_cfg.steps, realKernel);
        std::memcpy(channelData, grid.data(), S * sizeof(float));
    }

    for(size_t b = 0; b <= size_t(B); ++b ){
        const float *scratchBatch = scratch.data() + (b * elementsPerBatch);
        float *outBatch = output.data()  + (b * elementsPerBatch);
        comp::transpose(scratchBatch, outBatch, D, S);
    };
}
\
}
