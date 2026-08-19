#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <job_ggml.h>
#include <job_ggml_backend_sched.h>
#include <job_ggml_device_manager.h>

#include <job_model.h>
#include <job_token.h>

#include <chat/chat_message.h>
#include <config/arch/qwen/qwen3_instruct_2507.h>

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    const std::filesystem::path modelPath{
        "/srv/ai/ComfyUI/models/text_encoders/"
        "ZImage_Turbo/Qwen3-4B-Instruct-2507-Q4_K_M.gguf"
    };

    const std::filesystem::path tokenizerDir{
        "/home/jmills/git/opensource/qwen_tmp"
    };

    const std::filesystem::path tokenizerJson =
        tokenizerDir / "tokenizer.json";

    const std::filesystem::path tokenizerConfig =
        tokenizerDir / "tokenizer_config.json";

    //
    // ------------------------------------------------------------------------
    // Tokenizer.
    // ------------------------------------------------------------------------
    //

    if (!std::filesystem::exists(modelPath)) {
        std::cerr
            << "[JobInferenceTest] GGUF model not found: "
            << modelPath
            << '\n';

        return 1;
    }

    job::token::JobToken tokenizer;

    std::cout
        << "[JobInferenceTest] Loading tokenizer from "
        << tokenizerDir
        << "...\n";

    if (!tokenizer.load(
            job::token::IToken::Provider::HuggingFace,
            tokenizerConfig,
            tokenizerJson)) {

        std::cerr
            << "[JobInferenceTest] Failed to load tokenizer\n";

        return 1;
    }

    if (!tokenizer.isReady() ||
        !tokenizer.token()) {

        std::cerr
            << "[JobInferenceTest] Tokenizer runtime is not ready\n";

        return 1;
    }

    std::cout
        << "[JobInferenceTest] Tokenizer loaded. Vocab size: "
        << tokenizer.token()->vocabSize()
        << '\n';

    const std::string tokenizerRoundTrip =
        "The quick brown fox jumps over the lazy dog.";

    const std::vector<job::token::TokenId> tokenizerRoundTripTokens =
        tokenizer.encode(
            tokenizerRoundTrip);

    std::cout
        << "[JobInferenceTest] Tokenizer round trip: "
        << tokenizer.decode(
               tokenizerRoundTripTokens)
        << '\n';

    //
    // ------------------------------------------------------------------------
    // Resolve runtime.
    // ------------------------------------------------------------------------
    //

    job::ggml::JobGgml interface;
    job::ggml::JobGguf *gguf = interface.gguf();
    job::ggml::JobGgmlDeviceManager *manager = interface.deviceManager();

    if (!gguf) {
        std::cerr
            << "[JobInferenceTest] GGUF interface is unavailable\n";

        return 1;
    }

    if (!manager) {
        std::cerr
            << "[JobInferenceTest] Device manager is unavailable\n";

        return 1;
    }

    if (!manager->isValid()) {
        std::cerr
            << "[JobInferenceTest] Device manager failed: "
            << manager->errorString()
            << '\n';

        return 1;
    }

    //
    // ------------------------------------------------------------------------
    // Inspect the exact GGUF used for inference.
    // ------------------------------------------------------------------------
    //

    gguf->initParams()->setNoAlloc(true);
    gguf->initParams()->setCreateContext(false);

    if (!gguf->open(modelPath)) {
        std::cerr
            << "[JobInferenceTest] Failed to inspect GGUF model: "
            << modelPath
            << '\n';

        return 1;
    }

    if (!gguf->isValid() ||
        !gguf->hasContent()) {

        std::cerr
            << "[JobInferenceTest] GGUF metadata is invalid\n";

        return 1;
    }

    std::cout
        << "\n[JobInferenceTest] GGUF model metadata\n"
        << "  architecture:       "
        << gguf->readString(
               "general.architecture")
        << '\n'
        << "  name:               "
        << gguf->readString(
               "general.name")
        << '\n'
        << "  block count:        "
        << gguf->readInt(
               "qwen3.block_count")
        << '\n'
        << "  context length:     "
        << gguf->readInt(
               "qwen3.context_length")
        << '\n'
        << "  embedding length:   "
        << gguf->readInt(
               "qwen3.embedding_length")
        << '\n'
        << "  FFN length:         "
        << gguf->readInt(
               "qwen3.feed_forward_length")
        << '\n'
        << "  attention heads:    "
        << gguf->readInt(
               "qwen3.attention.head_count")
        << '\n'
        << "  KV heads:           "
        << gguf->readInt(
               "qwen3.attention.head_count_kv")
        << '\n'
        << "  key length:         "
        << gguf->readInt(
               "qwen3.attention.key_length")
        << '\n'
        << "  value length:       "
        << gguf->readInt(
               "qwen3.attention.value_length")
        << '\n'
        << "  RoPE dimension:     "
        << gguf->readInt(
               "qwen3.rope.dimension_count")
        << '\n'
        << "  RoPE base:          "
        << gguf->readFloat(
               "qwen3.rope.freq_base")
        << '\n'
        << "  RMS epsilon:        "
        << gguf->readFloat(
               "qwen3.attention.layer_norm_rms_epsilon")
        << '\n'
        << "  GGUF version:       "
        << gguf->version()
        << '\n'
        << "  alignment:          "
        << gguf->alignment()
        << '\n'
        << "  data offset:        "
        << gguf->dataOffset()
        << '\n'
        << "  key/value count:    "
        << gguf->keyValueCount()
        << '\n'
        << "  tensor count:       "
        << gguf->tensorCount()
        << '\n';

    //
    // ------------------------------------------------------------------------
    // Important tensor inventory.
    // ------------------------------------------------------------------------
    //

    const job::ggml::JobGgufContext *ggufContext =
        gguf->context();

    if (!ggufContext) {
        std::cerr
            << "[JobInferenceTest] GGUF context is unavailable\n";

        return 1;
    }

    static constexpr std::array RequiredTensors{
        "token_embd.weight",

        "blk.0.attn_norm.weight",
        "blk.0.attn_q.weight",
        "blk.0.attn_k.weight",
        "blk.0.attn_v.weight",
        "blk.0.attn_output.weight",
        "blk.0.attn_q_norm.weight",
        "blk.0.attn_k_norm.weight",

        "blk.0.ffn_norm.weight",
        "blk.0.ffn_gate.weight",
        "blk.0.ffn_up.weight",
        "blk.0.ffn_down.weight",

        "output_norm.weight"
    };

    std::cout
        << "\n[JobInferenceTest] Important GGUF tensors\n";

    for (const char *name : RequiredTensors) {
        if (!gguf->hasTensor(name)) {
            std::cerr
                << "  [MISSING] "
                << name
                << '\n';

            return 1;
        }

        const std::int64_t index =
            ggufContext->tensorIndex(
                name);

        if (index < 0) {
            std::cerr
                << "  [INVALID INDEX] "
                << name
                << '\n';

            return 1;
        }

        std::cout
            << "  "
            << ggufContext->tensorName(
                   index)
            << '\n'
            << "    index:  "
            << index
            << '\n'
            << "    type:   "
            << static_cast<std::uint32_t>(
                   ggufContext->tensorType(
                       index))
            << '\n'
            << "    bytes:  "
            << ggufContext->tensorSize(
                   index)
            << '\n'
            << "    offset: "
            << ggufContext->tensorOffset(
                   index)
            << '\n';
    }

    //
    // Qwen3-4B-Instruct-2507 ties the LM output projection to the token
    // embedding weights, so output.weight may legitimately be absent.
    //
    if (gguf->hasTensor(
            "output.weight")) {

        const std::int64_t index =
            ggufContext->tensorIndex(
                "output.weight");

        if (index < 0) {
            std::cerr
                << "[JobInferenceTest] output.weight has invalid index\n";

            return 1;
        }

        std::cout
            << "  output.weight\n"
            << "    index:  "
            << index
            << '\n'
            << "    type:   "
            << static_cast<std::uint32_t>(
                   ggufContext->tensorType(
                       index))
            << '\n'
            << "    bytes:  "
            << ggufContext->tensorSize(
                   index)
            << '\n'
            << "    offset: "
            << ggufContext->tensorOffset(
                   index)
            << '\n';
    } else {
        std::cout
            << "  output.weight: absent "
               "(tied token embeddings)\n";
    }

    //
    // ------------------------------------------------------------------------
    // Spot-check layer structure across the model.
    // ------------------------------------------------------------------------
    //

    static constexpr std::array LayerIndexes{
        0U,
        17U,
        35U
    };

    static constexpr std::array LayerTensorSuffixes{
        "attn_norm.weight",
        "attn_q.weight",
        "attn_k.weight",
        "attn_v.weight",
        "attn_output.weight",
        "attn_q_norm.weight",
        "attn_k_norm.weight",

        "ffn_norm.weight",
        "ffn_gate.weight",
        "ffn_up.weight",
        "ffn_down.weight"
    };

    std::cout
        << "\n[JobInferenceTest] GGUF layer structure\n";

    for (const std::uint32_t layerIndex :
         LayerIndexes) {

        std::cout
            << "  layer "
            << layerIndex
            << '\n';

        for (const char *suffix :
             LayerTensorSuffixes) {

            const std::string name =
                "blk." +
                std::to_string(
                    layerIndex) +
                "." +
                suffix;

            const bool present =
                gguf->hasTensor(
                    name);

            std::cout
                << "    "
                << (present
                        ? "[OK]      "
                        : "[MISSING] ")
                << name
                << '\n';

            if (!present)
                return 1;
        }
    }

    //
    // ------------------------------------------------------------------------
    // CPU runtime.
    // ------------------------------------------------------------------------
    //

    job::ggml::JobGgmlCpu *cpu =
        manager->cpu();

    if (!cpu) {
        std::cerr
            << "[JobInferenceTest] No CPU GGML device\n";

        return 1;
    }

    auto scheduler = manager->buildScheduler(cpu,4096);

    if (!scheduler || !scheduler->isValid()) {
        std::cerr << "[JobInferenceTest] Failed to create CPU scheduler\n";

        return 1;
    }

    std::cout
        << "\n[JobInferenceTest] Runtime resolved: "
        << cpu->uid()
        << '\n';

    //
    // ------------------------------------------------------------------------
    // Model configuration.
    // ------------------------------------------------------------------------
    //

    job::model::arch::qwen::Qwen3Instruct2507Config presetConfig;

    if (!presetConfig.isValid()) {
        std::cerr
            << "[JobInferenceTest] Qwen3 preset failed validation\n";

        return 1;
    }

    job::model::SamplerConfig samplerConfig = presetConfig.samplerConfig();
    // samplerConfig.setGreedy(true);
    std::cout
        << "\n[JobInferenceTest] Effective sampler configuration\n"
        << "  temperature:      "
        << samplerConfig.temperature()
        << '\n'
        << "  topK:             "
        << samplerConfig.topK()
        << '\n'
        << "  topP:             "
        << samplerConfig.topP()
        << '\n'
        << "  minP:             "
        << samplerConfig.minP()
        << '\n'
        << "  repeatPenalty:    "
        << samplerConfig.repeatPenalty()
        << '\n'
        << "  frequencyPenalty: "
        << samplerConfig.frequencyPenalty()
        << '\n'
        << "  presencePenalty:  "
        << samplerConfig.presencePenalty()
        << '\n'
        << "  seed:             "
        << samplerConfig.seed()
        << '\n'
        << "  greedy:           "
        << samplerConfig.greedy()
        << '\n';

    constexpr std::uint32_t InferenceContext = 4096;

    //
    // ------------------------------------------------------------------------
    // Model.
    // ------------------------------------------------------------------------
    //

    job::model::JobModel model{
        *cpu,
        *scheduler
    };

    std::cout
        << "[JobInferenceTest] Loading "
        << modelPath.filename()
        << "...\n";

    if (!model.load(
            modelPath,
            presetConfig,
            InferenceContext)) {

        std::cerr
            << "[JobInferenceTest] Failed to load model\n";

        return 1;
    }

    std::cout
        << "[JobInferenceTest] Model loaded\n"
        << "  architecture: "
        << model.config().architectureName()
        << '\n'
        << "  layers: "
        << model.config().transformerConfig().blockCount()
        << '\n'
        << "  context: "
        << model.kvCache()->maxContextLength()
        << '\n';

    //
    // ------------------------------------------------------------------------
    // Chat template.
    // ------------------------------------------------------------------------
    //

    job::token::ChatMessage systemMessage;

    systemMessage.setRole(
        job::token::ChatRole::System);

    systemMessage.setContent(
        "You are an expert C++ systems programmer. "
        "Provide high-performance, clean code without unnecessary explanations.");

    job::token::ChatMessage userMessage;

    userMessage.setRole(
        job::token::ChatRole::User);

    userMessage.setContent(
        "Write a high-performance C++ function that computes "
        "the inverse square root of a float.");

    const std::vector<job::token::ChatMessage> dialogue{
        systemMessage,
        userMessage
    };

    const std::string formattedPrompt =
        tokenizer.applyChatMessages(
            dialogue,
            true);

    if (formattedPrompt.empty()) {
        std::cerr
            << "[JobInferenceTest] Failed to apply chat messages\n";

        return 1;
    }

    std::cout
        << "\n[JobInferenceTest] Formatted prompt:\n"
        << formattedPrompt
        << "\n\n";

    const std::vector<job::token::TokenId> promptTokens =
        tokenizer.encode(
            formattedPrompt);

    if (promptTokens.empty()) {
        std::cerr
            << "[JobInferenceTest] Tokenizer produced no tokens\n";

        return 1;
    }

    std::cout
        << "[JobInferenceTest] Encoded "
        << promptTokens.size()
        << " prompt tokens\n";

    std::cout
        << "\n[JobInferenceTest] Prompt token IDs:\n";

    for (const job::token::TokenId token :
         promptTokens) {

        std::cout
            << token
            << ' ';
    }

    std::cout
        << "\n\n[JobInferenceTest] Prompt token pieces:\n";

    for (const job::token::TokenId id :
         promptTokens) {

        const auto token =
            tokenizer.token()->findTokenString(
                id);

        std::cout
            << id
            << " -> ["
            << (token
                    ? *token
                    : std::string_view{"<missing>"})
            << "]\n";
    }

    //
    // ------------------------------------------------------------------------
    // THIS IS THE KISS.
    // ------------------------------------------------------------------------
    //

    constexpr std::int32_t MaxNewTokens =
        64;

    const std::vector<job::token::TokenId> generatedTokens =
        model.generate(
            promptTokens,
            MaxNewTokens,
            samplerConfig);

    if (generatedTokens.empty()) {
        std::cerr
            << "[JobInferenceTest] Generation failed\n";

        return 1;
    }

    if (generatedTokens.size() <=
        promptTokens.size()) {

        std::cerr
            << "[JobInferenceTest] Model produced no response tokens\n";

        return 1;
    }

    //
    // JobModel returns prompt + generated continuation.
    //
    const auto responseBegin =
        generatedTokens.begin() +
        static_cast<std::ptrdiff_t>(
            promptTokens.size());

    const std::vector<job::token::TokenId> responseTokens{
        responseBegin,
        generatedTokens.end()
    };

    //
    // ------------------------------------------------------------------------
    // Inspect the generated token stream before decoding.
    // ------------------------------------------------------------------------
    //

    std::cout
        << "\n[JobInferenceTest] Generated response token IDs:\n";

    for (const job::token::TokenId token : responseTokens) {
        std::cout << token << ' ';
    }

    std::cout
        << '\n';

    //
    // ------------------------------------------------------------------------
    // Quick vocabulary inspection.
    // ------------------------------------------------------------------------
    //

    // const std::string input = "hello world";
    // const std::vector<job::token::TokenId> encoded = tokenizer.encode(input);

    // std::cout
    //     << "\nINPUT: ["
    //     << input
    //     << "]\n";

    // for (const job::token::TokenId id :
    //      encoded) {

    //     const auto token = tokenizer.token()->findTokenString(id);

    //     std::cout
    //         << id
    //         << " -> ["
    //         << (token
    //                 ? *token
    //                 : std::string_view{"<missing>"})
    //         << "]\n";
    // }

    //
    // ------------------------------------------------------------------------
    // Back through job_token.
    // ------------------------------------------------------------------------
    //

    const std::string responseText = tokenizer.decode(responseTokens);

    std::cout
        << "\n"
        << "==================================================\n"
        << "QWEN3 RESPONSE\n"
        << "==================================================\n"
        << responseText
        << '\n'
        << "==================================================\n"
        << "[JobInferenceTest] Complete\n";

    return 0;
}