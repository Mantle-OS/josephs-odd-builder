#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <format>
#include <string>
#include <string_view>

#include <config/arch/qwen/qwen3_instruct_2507.h>
#include <job_ggml_context.h>
#include <weights/model_weights.h>

using namespace job::model;
using namespace job::ggml;

struct ModelWeightsFixture
{
    ModelConfig config = arch::qwen::Qwen3Instruct2507Config{};
    JobGgmlContext::UPtr context;

    explicit ModelWeightsFixture(uint32_t layers = 2) :
        context{JobGgmlContext::createUniqMetadata(static_cast<std::size_t>(layers) * 16 + 16)}
    {
        config.transformerConfig().setBlockCount(layers);
        REQUIRE(context != nullptr);
        REQUIRE(context->isValid());
    }

    void tensor(std::string_view name)
    {
        auto tensor = context->newTensor2d(JobGgmlType::F32, 1, 1);
        REQUIRE(tensor != nullptr);
        REQUIRE(tensor->isValid());
        tensor->setName(std::string{name});
    }

    void globals(bool output = false)
    {
        tensor("token_embd.weight");
        tensor("output_norm.weight");

        if (output)
            tensor("output.weight");
    }

    void denseLayer(uint32_t layer, std::string_view omit = {})
    {
        const std::string prefix = std::format("blk.{}.", layer);

        for (const std::string &name : {
                 prefix + "attn_norm.weight",
                 prefix + "attn_q.weight",
                 prefix + "attn_k.weight",
                 prefix + "attn_v.weight",
                 prefix + "attn_output.weight",
                 prefix + "ffn_norm.weight",
                 prefix + "ffn_gate.weight",
                 prefix + "ffn_up.weight",
                 prefix + "ffn_down.weight"
             })
        {
            if (name != omit)
                tensor(name);
        }
    }

    void denseModel(bool output = false)
    {
        globals(output);

        for (uint32_t i = 0; i < config.transformerConfig().blockCount(); ++i)
            denseLayer(i);
    }
};

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("ModelWeights binds a dense transformer from a ggml context", "[model][weights][usage]")
{
    ModelWeightsFixture fixture;
    fixture.denseModel();

    ModelWeights weights;

    REQUIRE(weights.loadFromContext(*fixture.context, fixture.config));
    REQUIRE(weights.isLoaded());

    CHECK(weights.layerCount() == 2);
    CHECK(weights.tokenEmbd() != nullptr);
    CHECK(weights.outputNorm() != nullptr);

    for (std::size_t i = 0; i < weights.layerCount(); ++i) {
        const LayerWeights &layer = weights.layer(i);

        CHECK(layer.layerIndex == i);
        CHECK(layer.attnNorm != nullptr);
        CHECK(layer.attnQ != nullptr);
        CHECK(layer.attnK != nullptr);
        CHECK(layer.attnV != nullptr);
        CHECK(layer.attnOut != nullptr);
        CHECK(layer.ffnNorm != nullptr);
        CHECK(layer.ffnGate != nullptr);
        CHECK(layer.ffnUp != nullptr);
        CHECK(layer.ffnDown != nullptr);
    }
}

TEST_CASE("ModelWeights uses token embeddings as a tied output head", "[model][weights][usage]")
{
    ModelWeightsFixture fixture;
    fixture.denseModel();

    ModelWeights weights;

    REQUIRE(weights.loadFromContext(*fixture.context, fixture.config));

    CHECK(weights.hasTiedEmbedding());
    CHECK(weights.output() == weights.tokenEmbd());
}

TEST_CASE("ModelWeights binds an explicit output head when present", "[model][weights][usage]")
{
    ModelWeightsFixture fixture;
    fixture.denseModel(true);

    ModelWeights weights;

    REQUIRE(weights.loadFromContext(*fixture.context, fixture.config));

    CHECK_FALSE(weights.hasTiedEmbedding());
    REQUIRE(weights.output() != nullptr);
    CHECK(weights.output() != weights.tokenEmbd());
    CHECK(weights.output()->name() == "output.weight");
}

TEST_CASE("ModelWeights resolves HuggingFace tensor aliases", "[model][weights][usage]")
{
    ModelWeightsFixture fixture{1};

    fixture.tensor("model.embed_tokens.weight");
    fixture.tensor("model.norm.weight");

    fixture.tensor("model.layers.0.input_layernorm.weight");
    fixture.tensor("model.layers.0.self_attn.q_proj.weight");
    fixture.tensor("model.layers.0.self_attn.k_proj.weight");
    fixture.tensor("model.layers.0.self_attn.v_proj.weight");
    fixture.tensor("model.layers.0.self_attn.o_proj.weight");
    fixture.tensor("model.layers.0.post_attention_layernorm.weight");
    fixture.tensor("model.layers.0.mlp.gate_proj.weight");
    fixture.tensor("model.layers.0.mlp.up_proj.weight");
    fixture.tensor("model.layers.0.mlp.down_proj.weight");

    ModelWeights weights;

    REQUIRE(weights.loadFromContext(*fixture.context, fixture.config));
    REQUIRE(weights.isLoaded());

    CHECK(weights.tokenEmbd()->name() == "model.embed_tokens.weight");
    CHECK(weights.outputNorm()->name() == "model.norm.weight");

    const LayerWeights &layer = weights.layer(0);
    CHECK(layer.attnQ->name() == "model.layers.0.self_attn.q_proj.weight");
    CHECK(layer.ffnDown->name() == "model.layers.0.mlp.down_proj.weight");
}

// ============================================================================
// Block 2: Edge Cases / Invariants
// ============================================================================

TEST_CASE("ModelWeights starts and clears unloaded", "[model][weights][edge]")
{
    ModelWeightsFixture fixture;
    fixture.denseModel(true);

    ModelWeights weights;

    CHECK_FALSE(weights.isLoaded());
    CHECK(weights.layerCount() == 0);
    CHECK(weights.tokenEmbd() == nullptr);

    REQUIRE(weights.loadFromContext(*fixture.context, fixture.config));
    REQUIRE(weights.isLoaded());

    weights.clear();

    CHECK_FALSE(weights.isLoaded());
    CHECK(weights.layerCount() == 0);
    CHECK(weights.tokenEmbd() == nullptr);
    CHECK(weights.outputNorm() == nullptr);
    CHECK(weights.output() == nullptr);
}

TEST_CASE("ModelWeights rejects a missing token embedding", "[model][weights][edge]")
{
    ModelWeightsFixture fixture;

    fixture.tensor("output_norm.weight");
    fixture.denseLayer(0);
    fixture.denseLayer(1);

    ModelWeights weights;

    CHECK_FALSE(weights.loadFromContext(*fixture.context, fixture.config));
    CHECK_FALSE(weights.isLoaded());
    CHECK(weights.layerCount() == 0);
    CHECK(weights.tokenEmbd() == nullptr);
}

TEST_CASE("ModelWeights rejects a missing final normalization weight", "[model][weights][edge]")
{
    ModelWeightsFixture fixture;

    fixture.tensor("token_embd.weight");
    fixture.denseLayer(0);
    fixture.denseLayer(1);

    ModelWeights weights;

    CHECK_FALSE(weights.loadFromContext(*fixture.context, fixture.config));
    CHECK_FALSE(weights.isLoaded());
    CHECK(weights.layerCount() == 0);
}

TEST_CASE("ModelWeights rejects missing layer attention weights", "[model][weights][edge]")
{
    ModelWeightsFixture fixture;

    fixture.globals();
    fixture.denseLayer(0);
    fixture.denseLayer(1, "blk.1.attn_k.weight");

    ModelWeights weights;

    CHECK_FALSE(weights.loadFromContext(*fixture.context, fixture.config));
    CHECK_FALSE(weights.isLoaded());
    CHECK(weights.layerCount() == 0);
}

TEST_CASE("ModelWeights rejects missing dense FFN weights", "[model][weights][edge]")
{
    ModelWeightsFixture fixture;

    fixture.globals();
    fixture.denseLayer(0);
    fixture.denseLayer(1, "blk.1.ffn_down.weight");

    ModelWeights weights;

    CHECK_FALSE(weights.loadFromContext(*fixture.context, fixture.config));
    CHECK_FALSE(weights.isLoaded());
    CHECK(weights.layerCount() == 0);
}

TEST_CASE("ModelWeights failure clears previously loaded weights", "[model][weights][edge]")
{
    ModelWeightsFixture good;
    good.denseModel();

    ModelWeightsFixture bad;
    bad.globals();
    bad.denseLayer(0);
    bad.denseLayer(1, "blk.1.attn_v.weight");

    ModelWeights weights;

    REQUIRE(weights.loadFromContext(*good.context, good.config));
    REQUIRE(weights.isLoaded());
    REQUIRE(weights.layerCount() == 2);

    CHECK_FALSE(weights.loadFromContext(*bad.context, bad.config));

    CHECK_FALSE(weights.isLoaded());
    CHECK(weights.layerCount() == 0);
    CHECK(weights.tokenEmbd() == nullptr);
    CHECK(weights.outputNorm() == nullptr);
}

TEST_CASE("ModelWeights ignores absent optional tensors", "[model][weights][edge]")
{
    ModelWeightsFixture fixture;
    fixture.denseModel();

    ModelWeights weights;

    REQUIRE(weights.loadFromContext(*fixture.context, fixture.config));

    const LayerWeights &layer = weights.layer(0);

    CHECK(layer.attnNormBias == nullptr);
    CHECK(layer.attnQBias == nullptr);
    CHECK(layer.attnKBias == nullptr);
    CHECK(layer.attnVBias == nullptr);
    CHECK(layer.attnOutBias == nullptr);
    CHECK(layer.attnQNorm == nullptr);
    CHECK(layer.attnKNorm == nullptr);
    CHECK(layer.postAttnNorm == nullptr);
    CHECK(layer.ffnNormBias == nullptr);
    CHECK(layer.ffnGateBias == nullptr);
    CHECK(layer.ffnUpBias == nullptr);
    CHECK(layer.ffnDownBias == nullptr);
    CHECK(layer.postFfnNorm == nullptr);

    CHECK(weights.positionEmbd() == nullptr);
    CHECK(weights.typeEmbd() == nullptr);
}

TEST_CASE("ModelWeights layer lookup is bounds checked", "[model][weights][edge]")
{
    ModelWeightsFixture fixture;
    fixture.denseModel();

    ModelWeights weights;
    REQUIRE(weights.loadFromContext(*fixture.context, fixture.config));

    CHECK_NOTHROW(weights.layer(0));
    CHECK_NOTHROW(weights.layer(weights.layerCount() - 1));
    CHECK_THROWS_AS(weights.layer(weights.layerCount()), std::out_of_range);
}

TEST_CASE("ModelWeights requires MoE tensors when MoE is configured", "[model][weights][moe][edge]")
{
    ModelWeightsFixture fixture{1};

    MoeConfig moe;
    moe.setExpertCount(8);
    moe.setExpertUsedCount(2);
    fixture.config.setMoeConfig(moe);

    fixture.globals();

    fixture.tensor("blk.0.attn_norm.weight");
    fixture.tensor("blk.0.attn_q.weight");
    fixture.tensor("blk.0.attn_k.weight");
    fixture.tensor("blk.0.attn_v.weight");
    fixture.tensor("blk.0.attn_output.weight");
    fixture.tensor("blk.0.ffn_norm.weight");

    ModelWeights weights;

    CHECK_FALSE(weights.loadFromContext(*fixture.context, fixture.config));
    CHECK_FALSE(weights.isLoaded());

    fixture.tensor("blk.0.ffn_gate_inp.weight");
    fixture.tensor("blk.0.ffn_gate_exps.weight");
    fixture.tensor("blk.0.ffn_up_exps.weight");
    fixture.tensor("blk.0.ffn_down_exps.weight");

    REQUIRE(weights.loadFromContext(*fixture.context, fixture.config));
    CHECK(weights.isLoaded());

    const LayerWeights &layer = weights.layer(0);
    CHECK(layer.ffnGateInp != nullptr);
    CHECK(layer.ffnGateExps != nullptr);
    CHECK(layer.ffnUpExps != nullptr);
    CHECK(layer.ffnDownExps != nullptr);
}

// ============================================================================
// Block 3: Benchmarks / Stress
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark ModelWeights binding", "[model][weights][benchmark]")
{
    ModelWeightsFixture fixture{36};
    fixture.denseModel();

    BENCHMARK("Bind 36-layer Qwen3 weight metadata")
    {
        ModelWeights weights;
        return weights.loadFromContext(*fixture.context, fixture.config);
    };
}

#endif