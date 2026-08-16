#include <catch2/catch_test_macros.hpp>
#include "kv/kv_cache.h"
#include "config/model_config.h"

namespace job::model::test {

TEST_CASE("KvCache initializes, tracks sequence position, and clears cleanly", "[model][kv_cache]")
{
    ModelConfig config;
    config.m_transformerConfig.m_blockCount        = 4;
    config.m_transformerConfig.m_embeddingLength   = 512;
    config.m_transformerConfig.m_headCount         = 8;
    config.m_transformerConfig.m_contextLength     = 1024;
    config.m_transformerConfig.m_vocabSize         = 32000;

    KvCache kvCache;
    CHECK_FALSE(kvCache.isAllocated());

    bool ok = kvCache.init(config, 1024, ggml::JobGgmlType::F16);
    REQUIRE(ok);
    CHECK(kvCache.isAllocated());

    CHECK(kvCache.currentPosition() == 0);

    kvCache.advance(16);
    CHECK(kvCache.currentPosition() == 16);

    kvCache.advance(32);
    CHECK(kvCache.currentPosition() == 48);

    kvCache.clear();
    CHECK_FALSE(kvCache.isAllocated());
}

} // namespace job::model::test