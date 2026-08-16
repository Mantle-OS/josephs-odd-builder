#include <catch2/catch_test_macros.hpp>
#include "graph/compute_graph_builder.h"
#include "config/model_config.h"
#include "weights/model_weights.h"
#include "kv/kv_cache.h"
#include <job_ggml_context.h>

namespace job::model::test {

TEST_CASE("ComputeGraphBuilder constructs forward execution pipeline", "[model][graph]")
{
    ModelConfig config;
    config.m_transformerConfig.m_blockCount        = 2;
    config.m_transformerConfig.m_embeddingLength   = 256;
    config.m_transformerConfig.m_headCount         = 4;
    config.m_transformerConfig.m_contextLength     = 512;
    config.m_transformerConfig.m_vocabSize         = 1000;

    ModelWeights weights;
    KvCache kvCache;
    REQUIRE(kvCache.init(config, 512, ggml::JobGgmlType::F16));

    ComputeGraphBuilder builder(config, weights, kvCache);

    ggml::JobGgmlInitParams params(2 * 1024 * 1024, nullptr, false);
    auto ctx = ggml::JobGgmlContext::createUniq(params);
    REQUIRE(ctx->isValid());

    auto inputTensor = ctx->newTensor1d(ggml::JobGgmlType::I32, 8);
    REQUIRE(inputTensor != nullptr);

    uint32_t nPast = 0;

    // Verify graph construction invocation
    CHECK_NOTHROW(builder.buildForwardGraph(*ctx, *inputTensor, nPast));
}

} // namespace job::model::test