#include "weights/model_weights.h"

#include <format>
#include <job_logger.h>

namespace job::model {

void ModelWeights::clear() noexcept
{
    m_tokenEmbd.reset();
    m_outputNorm.reset();
    m_outputNormBias.reset();
    m_output.reset();
    m_positionEmbd.reset();
    m_typeEmbd.reset();
    m_layers.clear();
}

ggml::JobGgmlTensor::UPtr ModelWeights::findTensor(
    ggml::JobGgmlContext& context,
    std::string_view canonicalName,
    std::initializer_list<std::string_view> fallbackAliases) const
{
    auto tensor = context.tensor(std::string(canonicalName));
    if (tensor && tensor->isValid()) {
        return tensor;
    }

    for (const auto& alias : fallbackAliases) {
        tensor = context.tensor(std::string(alias));
        if (tensor && tensor->isValid()) {
            return tensor;
        }
    }

    return nullptr;
}

const LayerWeights& ModelWeights::layer(size_t index) const
{
    if (index >= m_layers.size()) {
        static const LayerWeights s_empty{};
        JOB_LOG_ERROR("[ModelWeights] Requested out-of-bounds layer index: {} (total layers: {})", index, m_layers.size());
        return s_empty;
    }
    return m_layers[index];
}

bool ModelWeights::loadFromContext(ggml::JobGgmlContext& context, const ModelConfig& config)
{
    clear();

    if (!context.isValid()) {
        JOB_LOG_ERROR("[ModelWeights] Cannot bind weights: JobGgmlContext is invalid");
        return false;
    }

    // 1. Global Token Embeddings & Normalization
    m_tokenEmbd = findTensor(context, "token_embd.weight", {"model.embed_tokens.weight", "tok_embeddings.weight"});
    if (!m_tokenEmbd) {
        JOB_LOG_ERROR("[ModelWeights] Missing required token embedding tensor: 'token_embd.weight'");
        return false;
    }

    m_outputNorm = findTensor(context, "output_norm.weight", {"model.norm.weight", "norm.weight"});
    if (!m_outputNorm) {
        JOB_LOG_ERROR("[ModelWeights] Missing required final output norm tensor: 'output_norm.weight'");
        return false;
    }
    m_outputNormBias = findTensor(context, "output_norm.bias", {"model.norm.bias", "norm.bias"});

    // Output LM Head (Optional - defaults to tokenEmbd if tied)
    m_output = findTensor(context, "output.weight", {"lm_head.weight"});

    // Position / Type Embeddings
    m_positionEmbd = findTensor(context, "position_embd.weight", {"model.embed_positions.weight"});
    m_typeEmbd     = findTensor(context, "token_types.weight");

    // Transformer Layer Blocks
    const uint32_t numLayers = config.transformerConfig().blockCount();
    m_layers.resize(numLayers);

    for (uint32_t i = 0; i < numLayers; ++i) {
        LayerWeights& lw = m_layers[i];
        lw.layerIndex = i;

        const std::string prefix = std::format("blk.{}.", i);

        // Attention Normalization
        lw.attnNorm     = findTensor(context, prefix + "attn_norm.weight", {std::format("model.layers.{}.input_layernorm.weight", i)});
        lw.attnNormBias = findTensor(context, prefix + "attn_norm.bias",   {std::format("model.layers.{}.input_layernorm.bias", i)});

        // Attention Projections
        lw.attnQ   = findTensor(context, prefix + "attn_q.weight",   {std::format("model.layers.{}.self_attn.q_proj.weight", i)});
        lw.attnK   = findTensor(context, prefix + "attn_k.weight",   {std::format("model.layers.{}.self_attn.k_proj.weight", i)});
        lw.attnV   = findTensor(context, prefix + "attn_v.weight",   {std::format("model.layers.{}.self_attn.v_proj.weight", i)});
        lw.attnOut = findTensor(context, prefix + "attn_output.weight", {
                                                                            prefix + "attn_out.weight",
                                                                            std::format("model.layers.{}.self_attn.o_proj.weight", i)
                                                                        });

        // Attention Biases
        lw.attnQBias   = findTensor(context, prefix + "attn_q.bias",   {std::format("model.layers.{}.self_attn.q_proj.bias", i)});
        lw.attnKBias   = findTensor(context, prefix + "attn_k.bias",   {std::format("model.layers.{}.self_attn.k_proj.bias", i)});
        lw.attnVBias   = findTensor(context, prefix + "attn_v.bias",   {std::format("model.layers.{}.self_attn.v_proj.bias", i)});
        lw.attnOutBias = findTensor(context, prefix + "attn_output.bias", {
                                                                              prefix + "attn_out.bias",
                                                                              std::format("model.layers.{}.self_attn.o_proj.bias", i)
                                                                          });

        // Q/K Normalization (Gemma 2 / Command-R)
        lw.attnQNorm = findTensor(context, prefix + "attn_q_norm.weight", {std::format("model.layers.{}.self_attn.q_norm.weight", i)});
        lw.attnKNorm = findTensor(context, prefix + "attn_k_norm.weight", {std::format("model.layers.{}.self_attn.k_norm.weight", i)});

        // Post-Attention Norm (Gemma 2)
        lw.postAttnNorm = findTensor(context, prefix + "post_attention_norm.weight", {std::format("model.layers.{}.post_attention_layernorm.weight", i)});

        // Feed-Forward Normalization
        lw.ffnNorm     = findTensor(context, prefix + "ffn_norm.weight", {std::format("model.layers.{}.post_attention_layernorm.weight", i)});
        lw.ffnNormBias = findTensor(context, prefix + "ffn_norm.bias",   {std::format("model.layers.{}.post_attention_layernorm.bias", i)});

        // Dense Feed-Forward Projections
        lw.ffnGate = findTensor(context, prefix + "ffn_gate.weight", {std::format("model.layers.{}.mlp.gate_proj.weight", i)});
        lw.ffnUp   = findTensor(context, prefix + "ffn_up.weight",   {std::format("model.layers.{}.mlp.up_proj.weight", i)});
        lw.ffnDown = findTensor(context, prefix + "ffn_down.weight", {std::format("model.layers.{}.mlp.down_proj.weight", i)});

        lw.ffnGateBias = findTensor(context, prefix + "ffn_gate.bias", {std::format("model.layers.{}.mlp.gate_proj.bias", i)});
        lw.ffnUpBias   = findTensor(context, prefix + "ffn_up.bias",   {std::format("model.layers.{}.mlp.up_proj.bias", i)});
        lw.ffnDownBias = findTensor(context, prefix + "ffn_down.bias", {std::format("model.layers.{}.mlp.down_proj.bias", i)});

        // Post-FFN Norm (Gemma 2)
        lw.postFfnNorm = findTensor(context, prefix + "post_ffw_norm.weight", {std::format("model.layers.{}.post_feedforward_layernorm.weight", i)});

        // Mixture of Experts (MoE). Presence of a MoeConfig is the signal
        // now, not a flat expertCount field on ArchConfig -- see
        // ModelConfig::hasMoeConfig().
        if (config.hasMoeConfig()) {
            lw.ffnGateInp  = findTensor(context, prefix + "ffn_gate_inp.weight",  {std::format("model.layers.{}.block_sparse_moe.gate.weight", i)});
            lw.ffnGateExps = findTensor(context, prefix + "ffn_gate_exps.weight", {std::format("model.layers.{}.block_sparse_moe.experts.gate.weight", i)});
            lw.ffnUpExps   = findTensor(context, prefix + "ffn_up_exps.weight",   {std::format("model.layers.{}.block_sparse_moe.experts.up.weight", i)});
            lw.ffnDownExps = findTensor(context, prefix + "ffn_down_exps.weight", {std::format("model.layers.{}.block_sparse_moe.experts.down.weight", i)});
        }

        // Verify minimum attention projection requirements
        if (!lw.attnNorm || !lw.attnQ || !lw.attnK || !lw.attnV || !lw.attnOut) {
            JOB_LOG_ERROR("[ModelWeights] Layer {} is missing required attention projections", i);
            return false;
        }
    }

    JOB_LOG_INFO("[ModelWeights] Successfully bound {} transformer layers (Tied Embeddings: {})",
                 m_layers.size(), hasTiedEmbedding());

    return true;
}

} // namespace job::model