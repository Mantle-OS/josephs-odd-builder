#include <catch2/catch_test_macros.hpp>
#include "weights/model_weights.h"
#include "config/model_config.h"
#include "job_gguf.h"
#include <job_ggml_context.h>

namespace job::model::test {

TEST_CASE("ModelWeights initializes unladen, handles invalid contexts, and clears cleanly", "[model][weights]")
{
    ModelWeights weights;
    CHECK_FALSE(weights.isLoaded());

    ModelConfig config;
    config.m_transformerConfig.m_blockCount = 2;
    config.m_transformerConfig.m_embeddingLength = 256;
    config.m_transformerConfig.m_headCount = 4;
    config.m_transformerConfig.m_vocabSize = 1000;

    // Creating an empty/default context to test safety guards
    ggml::JobGgmlInitParams params(1024, nullptr, false);
    auto emptyCtx = ggml::JobGgmlContext::createUniq(params);
    REQUIRE(emptyCtx != nullptr);

    // Attempting to load from an empty context lacking required weight tensors should fail gracefully
    bool ok = weights.loadFromContext(*emptyCtx, config);
    CHECK_FALSE(ok);
    CHECK_FALSE(weights.isLoaded());

    weights.clear();
    CHECK_FALSE(weights.isLoaded());
}

} // namespace job::model::test